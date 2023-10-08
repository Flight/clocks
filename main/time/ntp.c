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

void ntp_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Init started");
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  esp_netif_sntp_init(&config);

  ESP_LOGI(TAG, "Init done");

  while (1)
  {
    // Check if synchronized
    if (esp_netif_sntp_sync_wait(10000 / portTICK_PERIOD_MS) == ESP_OK)
    {
      ESP_LOGI(TAG, "Got the time");
      xEventGroupSetBits(global_event_group, IS_NTP_SET_BIT);

      // Check NTP time once in 24 hours
      vTaskDelay(86400000 / portTICK_PERIOD_MS);
    }
    else
    {
      printf("Failed to update system time within 10s timeout. Trying to repeat in 10 seconds.");
      vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
  }
}
