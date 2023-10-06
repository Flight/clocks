#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sntp.h>
#include "sdkconfig.h"
#include "global_event_group.h"

#define SNTP_SERVER "pool.ntp.org"

void ntp_task(void *pvParameter)
{
  xEventGroupWaitBits(global_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, SNTP_SERVER);
  esp_sntp_init();

  while (1)
  {
    // Check if synchronized
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
    {
      xEventGroupSetBits(global_event_group, IS_NTP_SET_BIT);
      // Check NTP time once in 24 hours
      vTaskDelay(86400000 / portTICK_PERIOD_MS);
    }
    else
    {
      // If not synchronized, delay for a shorter period and check again.
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}
