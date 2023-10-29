#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>
#include <esp_netif.h>

#include "sdkconfig.h"
#include "global_event_group.h"

#include "ntp.h"

#define SNTP_SERVER "pool.ntp.org"

static const char *TAG = "NTP";
static const uint8_t REFRESH_INTERVAL_HOURS = 24;
static const uint8_t UPDATE_TIMEOUT_SECS = 10;
static const uint8_t RETRY_SECS = 10;
static const uint8_t MAX_RETRIES = 3;
static uint8_t retry_count = 0;

void ntp_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Init started");
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  esp_netif_sntp_init(&config);
  ESP_LOGI(TAG, "Init done");

  while (true)
  {
    while (retry_count < MAX_RETRIES)
    {
      esp_err_t err = esp_netif_sntp_sync_wait(1000 * UPDATE_TIMEOUT_SECS / portTICK_PERIOD_MS);

      if (err == ESP_OK)
      {
        ESP_LOGI(TAG, "Got the time, next time sync in %d hours.", REFRESH_INTERVAL_HOURS);
        xEventGroupSetBits(global_event_group, IS_TIME_FROM_NPT_UP_TO_DATE_BIT);
        break;
      }

      ESP_LOGW(TAG, "Failed to update system time within %d seconds. (attempt %d of %d)", UPDATE_TIMEOUT_SECS, retry_count + 1, MAX_RETRIES);
      if (retry_count + 1 < MAX_RETRIES)
      {
        ESP_LOGI(TAG, "Trying to repeat in %d seconds.", RETRY_SECS);
      }

      vTaskDelay(1000 * RETRY_SECS / portTICK_PERIOD_MS);
      retry_count++;

      if (retry_count == MAX_RETRIES)
      {
        ESP_LOGE(TAG, "Can't fetch the time. Next retry in %d hours.", REFRESH_INTERVAL_HOURS);
      }
    }

    vTaskDelay(1000 * 60 * 60 * REFRESH_INTERVAL_HOURS / portTICK_PERIOD_MS);
  }
}
