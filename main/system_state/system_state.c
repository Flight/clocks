#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <stdlib.h>
#include <esp_heap_caps.h>

static const u_int8_t UPDATE_INTERVAL_MINS = 2;
static const u_int8_t AUTO_RESTART_IF_HEAP_LESS_KB = 50;

static const char *TASK_STATES[] = {
    "Running",   // eRunning
    "Ready",     // eReady
    "Blocked",   // eBlocked
    "Suspended", // eSuspended
    "Deleted",   // eDeleted
    "Invalid"    // eInvalid
};

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

static void printSystemState(void)
{
  static const char *TAG = "SYSTEM_STATE";

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

    int free_heap = esp_get_free_heap_size();
    int total_heap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    float percentage = (float)free_heap / total_heap * 100;

    ESP_LOGI(
        TAG,
        "DRAM left %dKB of %dKB (%.2f%%)",
        free_heap / 1024,
        total_heap / 1024,
        percentage);

    // Showing system uptime
    uint32_t system_uptime = esp_log_timestamp() / 1000;
    uint8_t system_uptime_secs = system_uptime % 60;
    uint8_t system_uptime_mins = (system_uptime / 60) % 60;
    uint8_t system_uptime_hours = (system_uptime / 3600) % 24;
    uint32_t system_uptime_days = (system_uptime / 86400);
    ESP_LOGI(
        TAG,
        "System uptime: %lu days, %d hours, %d minutes, %d seconds",
        system_uptime_days,
        system_uptime_hours,
        system_uptime_mins,
        system_uptime_secs);

    // The array is no longer needed, free the memory it consumes.
    vPortFree(pxTaskStatusArray);

    if (free_heap / 1024 < AUTO_RESTART_IF_HEAP_LESS_KB)
    {
      // Uncomment if the code has memory leaks
      // esp_restart();
    }
  }
}

void system_state_task(void *pvParameter)
{
  while (true)
  {
    printSystemState();
    vTaskDelay(1000 * 60 * UPDATE_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}