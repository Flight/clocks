#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "tm1637.h"
#include "global_event_group.h"

static const char *TAG = "Display";

static const gpio_num_t DISPLAY_CLK = CONFIG_TM1637_CLK_PIN;
static const gpio_num_t DISPLAY_DTA = CONFIG_TM1637_DIO_PIN;

static int MIN_BRIGHTNESS = 1;
static int MAX_BRIGHTNESS = 7;

void lcd_tm1637_task(void *pvParameter)
{
  tm1637_led_t *lcd = tm1637_init(DISPLAY_CLK, DISPLAY_DTA);

  tm1637_set_brightness(lcd, MIN_BRIGHTNESS);
  ESP_LOGI(TAG, "Init done");

  uint8_t seg_data[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20};
  for (uint8_t x = 0; x < 32; ++x)
  {
    uint8_t v_seg_data = seg_data[x % 6];
    tm1637_set_segment_raw(lcd, 0, v_seg_data);
    tm1637_set_segment_raw(lcd, 1, v_seg_data);
    tm1637_set_segment_raw(lcd, 2, v_seg_data);
    tm1637_set_segment_raw(lcd, 3, v_seg_data);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  ESP_LOGI(TAG, "Waiting for time");
  xEventGroupWaitBits(global_event_group, IS_NTP_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Got the time");

  while (true)
  {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to Amsterdam
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;

    ESP_LOGI(TAG, "Time is %i:%i", hours, minutes);

    if (hours >= 23 || hours <= 7)
    {
      tm1637_set_brightness(lcd, MIN_BRIGHTNESS);
    }
    else
    {
      tm1637_set_brightness(lcd, MAX_BRIGHTNESS);
    }

    tm1637_set_segment_number(lcd, 0, hours / 10, false);
    tm1637_set_segment_number(lcd, 1, hours % 10, true);
    tm1637_set_segment_number(lcd, 2, minutes / 10, false);
    tm1637_set_segment_number(lcd, 3, minutes % 10, false);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
