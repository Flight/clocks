#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>

// Assuming your log tag is defined somewhere
#define TAG "Auto Restart"

const uint8_t TIMEOUT_HOURS = 4;

void auto_restart_task(void *pvParameter)
{
  while (true)
  {
    ESP_LOGI(TAG, "Waiting for %u hours before restart", TIMEOUT_HOURS);
    vTaskDelay(1000 * 60 * 60 * TIMEOUT_HOURS / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Restarting now!");
    esp_restart();
  }
}