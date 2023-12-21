#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <ds3231.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "sdkconfig.h"
#include "global_event_group.h"

#include "external_timer.h"

static const char *TAG = "DS3231 Clocks";

static const gpio_num_t DS3231_SDA_PIN = CONFIG_SDA_PIN;
static const gpio_num_t DS3231_SCL_PIN = CONFIG_SCL_PIN;

static const uint8_t TIME_REFRESH_INTERVAL_MINS = 30;

static i2c_dev_t timer;

static void save_system_time_to_clocks()
{
  time_t now;
  struct tm time_from_ntp;
  char strftime_buf[64];

  time(&now);
  localtime_r(&now, &time_from_ntp);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &time_from_ntp);

  if (ds3231_set_time(&timer, &time_from_ntp) != ESP_OK)
  {
    ESP_LOGE(TAG, "Could not save time to the clocks. Module is not connected or the CR2032 battery is low.");
  }
  else
  {
    ESP_LOGI(TAG, "Time saved to the clocks: %s", strftime_buf);
  }
}

static void copy_time_from_clocks_to_sytem_time()
{
  struct tm time_from_clocks;
  if (ds3231_get_time(&timer, &time_from_clocks) != ESP_OK)
  {
    ESP_LOGE(TAG, "Could not get time. Module is not connected or the CR2032 battery is low.");
    return;
  }

  time_t timestamp = mktime(&time_from_clocks);

  ESP_LOGI(TAG, "Timestamp from clocks (secs) : %lld", (long long)timestamp);
  struct timeval now = {
      .tv_sec = timestamp,
      .tv_usec = 0};
  // The timezone was already set by main, so the second parameter is NULL
  settimeofday(&now, NULL);
}

void external_timer_task(void *pvParameters)
{
  ESP_LOGI(TAG, "Init");
  ds3231_init_desc(&timer, 0, DS3231_SDA_PIN, DS3231_SCL_PIN);

  while (true)
  {
    EventBits_t uxBits = xEventGroupGetBits(global_event_group);

    if (uxBits & IS_TIME_FROM_NPT_UP_TO_DATE_BIT)
    {
      ESP_LOGI(TAG, "Saving NTP time to clocks");
      save_system_time_to_clocks();
      xEventGroupClearBits(global_event_group, IS_TIME_FROM_NPT_UP_TO_DATE_BIT);
    }
    else
    {
      ESP_LOGI(TAG, "Applying the precise time from clocks");
      copy_time_from_clocks_to_sytem_time();
    }

    xEventGroupSetBits(global_event_group, IS_TIME_SET_BIT);

    vTaskDelay(1000 * 60 * TIME_REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}
