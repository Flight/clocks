#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <stdlib.h>
#include <esp_heap_caps.h>
#include <nvs.h>

static const char *NVS_STORAGE_NAMESPACE = "system_info";
static const char *NVS_UPTIME_KEY = "uptime_bfr_heap";
static const char *TAG = "System State";

static const uint8_t UPDATE_INTERVAL_MINS = 2;
static const uint8_t AUTO_RESTART_IF_HEAP_LESS_KB = 70; // The minimum needed RAM for OTA update is 70KB

static const char *TASK_STATES[] = {
    "Running",   // eRunning
    "Ready",     // eReady
    "Blocked",   // eBlocked
    "Suspended", // eSuspended
    "Deleted",   // eDeleted
    "Invalid"    // eInvalid
};

static nvs_handle_t uptime_storage_handle;

static int compare_tasks_by_runtime(const void *a, const void *b)
{
  const TaskStatus_t *taskA = (const TaskStatus_t *)a;
  const TaskStatus_t *taskB = (const TaskStatus_t *)b;

  if (taskA->ulRunTimeCounter > taskB->ulRunTimeCounter)
    return -1;
  if (taskA->ulRunTimeCounter < taskB->ulRunTimeCounter)
    return 1;
  return 0;
}

static void print_timestamp(uint32_t timestamp, const char *customMessage)
{
  uint32_t system_uptime = timestamp / 1000;
  uint8_t system_uptime_secs = system_uptime % 60;
  uint8_t system_uptime_mins = (system_uptime / 60) % 60;
  uint8_t system_uptime_hours = (system_uptime / 3600) % 24;
  uint32_t system_uptime_days = (system_uptime / 86400);
  ESP_LOGI(
      TAG,
      "%s%lu days, %d hours, %d minutes, %d seconds",
      customMessage,
      system_uptime_days,
      system_uptime_hours,
      system_uptime_mins,
      system_uptime_secs);
}

static void print_tasks_list()
{
  TaskStatus_t *pxTaskStatusArray;
  volatile UBaseType_t uxArraySize, x;
  uint32_t ulTotalRunTime;

  // Take a snapshot of the number of tasks in case it changes while this function is executing.
  uxArraySize = uxTaskGetNumberOfTasks();

  // Allocate memory to hold the task information.
  pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL)
  {
    // Generate the (binary) data.
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    qsort(pxTaskStatusArray, uxArraySize, sizeof(TaskStatus_t), compare_tasks_by_runtime);

    // Print the header.
    ESP_LOGI(TAG, "%-20s %-9s %4s %12s %5s %10s %7s",
             "Task Name",
             "State",
             "Prio",
             "Stack left",
             "Task#",
             "Runtime",
             "Percent");

    // Display the information about each task.
    for (x = 0; x < uxArraySize; x++)
    {
      ESP_LOGI(TAG, "%-20s %-9s %4u %12lu %5u %10lu %6lu%%",
               pxTaskStatusArray[x].pcTaskName,
               TASK_STATES[pxTaskStatusArray[x].eCurrentState],
               pxTaskStatusArray[x].uxCurrentPriority,
               pxTaskStatusArray[x].usStackHighWaterMark,
               pxTaskStatusArray[x].xTaskNumber,
               pxTaskStatusArray[x].ulRunTimeCounter,
               ulTotalRunTime ? (pxTaskStatusArray[x].ulRunTimeCounter * 100 / ulTotalRunTime) : 0);
    }
  }

  vPortFree(pxTaskStatusArray);
}

void show_last_uptime_before_out_of_memory()
{
  uint32_t last_uptime_before_out_of_memory;
  size_t required_size = sizeof(last_uptime_before_out_of_memory);
  esp_err_t err;

  err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &uptime_storage_handle);
  if (err != ESP_OK)
  {
    return;
  }
  err = nvs_get_blob(uptime_storage_handle, NVS_UPTIME_KEY, &last_uptime_before_out_of_memory, &required_size);
  nvs_close(uptime_storage_handle);
  if (err != ESP_OK)
  {
    return;
  }

  print_timestamp(last_uptime_before_out_of_memory, "Last uptime before out of heap memory: ");
}

static void print_system_state()
{
  uint32_t system_uptime = esp_log_timestamp();
  uint32_t free_heap = esp_get_free_heap_size();
  uint32_t total_heap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  float percentage = (float)free_heap / total_heap * 100;

  print_tasks_list();

  ESP_LOGI(
      TAG,
      "DRAM left %luKB of %luKB (%.2f%%)",
      free_heap / 1024,
      total_heap / 1024,
      percentage);

  print_timestamp(system_uptime, "System uptime: ");
}

static void restart_if_free_heap_low()
{
  uint32_t free_heap_kb = esp_get_free_heap_size() / 1024;
  uint32_t system_uptime = esp_log_timestamp();
  esp_err_t err;

  if (free_heap_kb / 1024 < AUTO_RESTART_IF_HEAP_LESS_KB)
  {
    // Save the system uptime to the RTC memory
    err = nvs_open("storage", NVS_READWRITE, &uptime_storage_handle);
    if (err != ESP_OK)
    {
      return;
    }
    ESP_LOGE(TAG, "Out of heap memory (%lu KB)! Restarting...", free_heap_kb);
    nvs_set_blob(uptime_storage_handle, NVS_UPTIME_KEY, &system_uptime, sizeof(system_uptime));
    nvs_commit(uptime_storage_handle);
    nvs_close(uptime_storage_handle);
    esp_restart();
  }
}

void system_state_task(void *pvParameter)
{
  show_last_uptime_before_out_of_memory();

  while (true)
  {
    print_system_state();
    restart_if_free_heap_low();

    vTaskDelay(1000 * 60 * UPDATE_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}