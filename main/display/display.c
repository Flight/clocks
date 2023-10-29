#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "sdkconfig.h"
#include "tm1637.h"
#include "global_event_group.h"

#include "display.h"

static const char *TAG = "Display";

static const gpio_num_t DISPLAY_CLK = CONFIG_TM1637_CLK_PIN;
static const gpio_num_t DISPLAY_DTA = CONFIG_TM1637_DIO_PIN;

uint8_t MIN_BRIGHTNESS = 1;
uint8_t MAX_BRIGHTNESS = 7;
static uint8_t current_brightness = 1;
static uint8_t SEGMENTS_AMOUNT = 4;
static uint8_t SECONDS_UNTIL_TEMPERATURE_SHOWN = 20;
static uint8_t SECONDS_TO_SHOW_TEMPERATURE = 5;

tm1637_led_t *lcd;

static void show_dashes()
{
  for (uint8_t segment = 0; segment < SEGMENTS_AMOUNT; segment++)
  {
    tm1637_set_segment_raw(lcd, segment, 0x40);
  }
}

static void check_segments()
{
  uint8_t seg_data[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20};
  for (uint8_t x = 0; x < 32; ++x)
  {
    uint8_t v_seg_data = seg_data[x % 6];
    for (uint8_t segment = 0; segment < SEGMENTS_AMOUNT; segment++)
    {
      tm1637_set_segment_raw(lcd, segment, v_seg_data);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  show_dashes();
}

static void change_brightness_smoothly(int new_brightness)
{
  int8_t difference = new_brightness - current_brightness;

  for (uint8_t brightness_step = 0; brightness_step < abs(difference); brightness_step++)
  {
    show_dashes();

    if (difference > 0)
    {
      current_brightness++;
    }
    else
    {
      current_brightness--;
    }
    tm1637_set_brightness(lcd, current_brightness);
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

static void show_time(uint8_t hours, uint8_t minutes, bool is_column_on)
{
  tm1637_set_segment_number(lcd, 0, hours / 10, false);
  tm1637_set_segment_number(lcd, 1, hours % 10, is_column_on);
  tm1637_set_segment_number(lcd, 2, minutes / 10, false);
  tm1637_set_segment_number(lcd, 3, minutes % 10, false);
}

static void show_temperature(float temperature)
{
  int8_t rounded_temperature = (int8_t)roundf(temperature);
  char temperature_string[5];

  // Convert the temperature to string
  sprintf(temperature_string, "%d", rounded_temperature);

  int8_t index = strlen(temperature_string) - 1;

  for (int8_t segment_index = SEGMENTS_AMOUNT - 2; segment_index >= 0; segment_index--, index--)
  {
    if (index < 0)
    {
      // Clear segment
      tm1637_set_segment_raw(lcd, segment_index, 0);
      continue;
    }

    if (temperature_string[index] == '-')
    {
      // Minus sign
      tm1637_set_segment_raw(lcd, segment_index, 0x40);
      continue;
    }

    // Digit
    tm1637_set_segment_number(lcd, segment_index, temperature_string[index] - '0', false);
  }

  // degree sign
  tm1637_set_segment_raw(lcd, 3, 0xe3);
}

void lcd_tm1637_task(void *pvParameter)
{
  bool is_column_on = true;
  u_int8_t seconds_time_shown = 0;
  lcd = tm1637_init(DISPLAY_CLK, DISPLAY_DTA);

  tm1637_set_brightness(lcd, current_brightness);
  ESP_LOGI(TAG, "Init done");

  check_segments();

  ESP_LOGI(TAG, "Waiting for time");
  xEventGroupWaitBits(global_event_group, IS_TIME_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Got the time");

  while (true)
  {
    EventBits_t uxBits = xEventGroupGetBits(global_event_group);

    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    seconds_time_shown++;

    time(&now);

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    uint8_t hours = timeinfo.tm_hour;
    uint8_t minutes = timeinfo.tm_min;

    if (uxBits & IS_LIGHT_SENSOR_READING_DONE_BIT)
    {
      if (global_is_light_on)
      {
        change_brightness_smoothly(MAX_BRIGHTNESS);
      }
      else
      {
        change_brightness_smoothly(MIN_BRIGHTNESS);
      }
    }

    if (seconds_time_shown > SECONDS_UNTIL_TEMPERATURE_SHOWN)
    {
      if (uxBits & IS_INSIDE_TEMPERATURE_READING_DONE_BIT && global_inside_temperature)
      {
        show_temperature(global_inside_temperature);
        vTaskDelay(SECONDS_TO_SHOW_TEMPERATURE * 1000 / portTICK_PERIOD_MS);
      }

      if (uxBits & IS_OUTSIDE_TEMPERATURE_READING_DONE_BIT && global_outside_temperature)
      {
        show_temperature(global_outside_temperature);
        vTaskDelay(SECONDS_TO_SHOW_TEMPERATURE * 1000 / portTICK_PERIOD_MS);
      }

      seconds_time_shown = 0;
      continue;
    }

    show_time(hours, minutes, is_column_on);
    is_column_on = !is_column_on;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}