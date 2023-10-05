#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sntp.h>
#include "sdkconfig.h"

#include "ntp_event.h"

#define SNTP_SERVER "pool.ntp.org"

void ntp_task(void *pvParameter)
{
  while (1)
  {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, SNTP_SERVER);
    esp_sntp_init();
    xEventGroupSetBits(ntp_event_group, IS_NTP_SET_BIT);
    // Check NTP time once in 24 hours
    vTaskDelay(86400000 / portTICK_PERIOD_MS);
  }
}
