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

static const gpio_num_t CLK_PIN = CONFIG_TM1637_CLK_PIN;
static const gpio_num_t DTA_PIN = CONFIG_TM1637_DIO_PIN;

static const uint8_t MIN_BRIGHTNESS = 0;
static const uint8_t MED_BRIGHTNESS = 2;
static const uint8_t MAX_BRIGHTNESS = 7;
static uint8_t current_brightness = 0;

static const uint8_t DIGITS_AMOUNT = 4;
static const uint8_t SECONDS_UNTIL_TEMPERATURE_SHOWN = 20;
static const uint8_t SECONDS_TO_SHOW_EACH_TEMPERATURE = 5;
static const uint8_t CIRCLE_LOOP_LENGTH = 32;

static uint8_t current_hours = 0;
static uint8_t current_minutes = 0;
static float current_temperature = 0.0;
static bool current_is_column_on = false;
static bool display_temperature = false;

static tm1637_led_t *lcd;

static void show_dashes()
{
  for (uint8_t current_digit_index = 0; current_digit_index < DIGITS_AMOUNT; current_digit_index++)
  {
    tm1637_set_segment_raw(lcd, current_digit_index, 0x40);
  }
}

static void check_segments()
{
  uint8_t segments_of_circle[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20};
  for (uint8_t loop_index = 0; loop_index < CIRCLE_LOOP_LENGTH; ++loop_index)
  {
    uint8_t current_segment_of_circle = segments_of_circle[loop_index % 6];
    for (uint8_t current_digit = 0; current_digit < DIGITS_AMOUNT; current_digit++)
    {
      tm1637_set_segment_raw(lcd, current_digit, current_segment_of_circle);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  show_dashes();
}

static void show_time(uint8_t hours, uint8_t minutes, bool is_column_on)
{
  current_hours = hours;
  current_minutes = minutes;
  current_is_column_on = is_column_on;
  display_temperature = false;

  tm1637_set_segment_number(lcd, 0, hours / 10, false);
  tm1637_set_segment_number(lcd, 1, hours % 10, is_column_on);
  tm1637_set_segment_number(lcd, 2, minutes / 10, false);
  tm1637_set_segment_number(lcd, 3, minutes % 10, false);
}

static void show_temperature(float temperature)
{
  current_temperature = temperature;
  display_temperature = true;

  int32_t rounded_temperature = (int32_t)roundf(temperature);
  char temperature_string[5];

  // Convert the temperature to string
  sprintf(temperature_string, "%ld", rounded_temperature);

  int8_t index = strlen(temperature_string) - 1;

  for (int8_t current_digit_index = DIGITS_AMOUNT - 2; current_digit_index >= 0; current_digit_index--, index--)
  {
    if (index < 0)
    {
      // Clear segment
      tm1637_set_segment_raw(lcd, current_digit_index, 0);
      continue;
    }

    if (temperature_string[index] == '-')
    {
      // Minus sign
      tm1637_set_segment_raw(lcd, current_digit_index, 0x40);
      continue;
    }

    // Digit
    tm1637_set_segment_number(lcd, current_digit_index, temperature_string[index] - '0', false);
  }

  // Degree sign
  tm1637_set_segment_raw(lcd, 3, 0xe3);
}

static void change_brightness_smoothly(uint8_t new_brightness)
{
  int8_t difference = new_brightness - current_brightness;

  for (uint8_t brightness_step = 0; brightness_step < abs(difference); brightness_step++)
  {
    if (difference > 0)
    {
      current_brightness++;
    }
    else
    {
      current_brightness--;
    }
    tm1637_set_brightness(lcd, current_brightness);

    if (display_temperature)
    {
      show_temperature(current_temperature);
    }
    else
    {
      show_time(current_hours, current_minutes, current_is_column_on);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void check_brightness()
{
  uint8_t target_brightness = current_brightness;

  switch (global_light_level_index)
  {
  case LIGHT_LEVEL_LOW:
    target_brightness = MIN_BRIGHTNESS;
    break;
  case LIGHT_LEVEL_MEDIUM:
    target_brightness = MED_BRIGHTNESS;
    break;
  case LIGHT_LEVEL_HIGH:
    target_brightness = MAX_BRIGHTNESS;
    break;
  }

  if (target_brightness != current_brightness)
  {
    change_brightness_smoothly(target_brightness);
  }
}

void lcd_tm1637_task(void *pvParameter)
{
  bool is_column_on = false;
  uint8_t seconds_time_shown = 0;
  uint8_t seconds_temperature_shown = 0;
  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  lcd = tm1637_init(CLK_PIN, DTA_PIN);

  tm1637_set_brightness(lcd, MIN_BRIGHTNESS);
  ESP_LOGI(TAG, "Init done");

  check_segments();

  ESP_LOGI(TAG, "Waiting for time");
  xEventGroupWaitBits(global_event_group, IS_TIME_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Got the time");

  while (true)
  {
    EventBits_t uxBits = xEventGroupGetBits(global_event_group);

    time(&now);

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    uint8_t hours = timeinfo.tm_hour;
    uint8_t minutes = timeinfo.tm_min;

    if ((display_temperature || seconds_time_shown > SECONDS_UNTIL_TEMPERATURE_SHOWN) && seconds_temperature_shown < SECONDS_TO_SHOW_EACH_TEMPERATURE * 2)
    {
      display_temperature = true;
      if (uxBits & IS_INSIDE_TEMPERATURE_READING_DONE_BIT && global_inside_temperature != -1000 && seconds_temperature_shown < SECONDS_TO_SHOW_EACH_TEMPERATURE)
      {
        show_temperature(global_inside_temperature);
      }
      else if (uxBits & IS_OUTSIDE_WEATHER_READING_DONE_BIT && global_outside_temperature != -1000 && seconds_temperature_shown < SECONDS_TO_SHOW_EACH_TEMPERATURE * 2)
      {
        show_temperature(global_outside_temperature);
      }

      seconds_temperature_shown++;
      seconds_time_shown = 0;

      if (uxBits & IS_LIGHT_SENSOR_READING_DONE_BIT)
      {
        check_brightness();
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    display_temperature = false;
    seconds_time_shown++;
    seconds_temperature_shown = 0;
    show_time(hours, minutes, is_column_on);
    is_column_on = !is_column_on;

    if (uxBits & IS_LIGHT_SENSOR_READING_DONE_BIT)
    {
      check_brightness();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}