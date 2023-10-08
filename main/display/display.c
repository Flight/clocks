#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <stdlib.h>

#include "sdkconfig.h"
#include "tm1637.h"
#include "global_event_group.h"

static const char *TAG = "Display";

static const gpio_num_t DISPLAY_CLK = CONFIG_TM1637_CLK_PIN;
static const gpio_num_t DISPLAY_DTA = CONFIG_TM1637_DIO_PIN;

uint8_t MIN_BRIGHTNESS = 1;
uint8_t MAX_BRIGHTNESS = 7;
static int8_t current_brightness = 1;

tm1637_led_t *lcd;

static void check_segments()
{
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
}

static void change_brightness_smoothly(int new_brightness)
{
  int8_t difference = new_brightness - current_brightness;

  for (uint8_t brightness_step = 0; brightness_step < abs(difference); brightness_step++)
  {
    tm1637_set_number(lcd, 8888);
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

void static clear_segments()
{
  for (uint8_t segment = 0; segment < 4; segment++)
  {
    tm1637_set_segment_raw(lcd, segment, 0x00);
  }
}

void show_digit_with_dot(uint8_t digit, uint8_t position)
{
  // Array to map digits to their 7-segment encodings
  static const uint8_t segment_map[] = {
      0x3F, 0x06, 0x5B, 0x4F,
      0x66, 0x6D, 0x7D, 0x07,
      0x7F, 0x6F};

  // Get the raw segment value for the digit
  uint8_t raw_value = segment_map[digit];

  // Add the dot by setting the least significant bit
  raw_value |= 0x01;

  tm1637_set_segment_raw(lcd, position, raw_value);
}

static void show_temperature(float temperature)
{
  uint8_t temp_whole = (uint8_t)temperature; // Extract whole number
  uint8_t first_whole_digit = temp_whole / 10;
  uint8_t second_whole_digit = temp_whole % 10;
  uint8_t fraction = (uint8_t)temperature * 10 % 10; // Extract one decimal place

  // Handle negative temperatures
  bool is_negative = (temperature < 0);

  clear_segments();

  if (is_negative)
  {
    tm1637_set_segment_raw(lcd, 0, 0x40);                        // Display minus sign
    tm1637_set_segment_number(lcd, 1, first_whole_digit, false); // Display tens place
    tm1637_set_segment_number(lcd, 2, second_whole_digit, true); // Display units place with decimal point
    tm1637_set_segment_number(lcd, 3, fraction, false);          // Display tenths place after the decimal point
  }
  else
  {
    tm1637_set_segment_number(lcd, 0, first_whole_digit / 10, false); // Display tens place
    tm1637_set_segment_number(lcd, 1, second_whole_digit, true);      // Display units place with decimal point
    tm1637_set_segment_number(lcd, 2, fraction, false);               // Display tenths place after the decimal point
    tm1637_set_segment_raw(lcd, 3, 0x46);                             // Display small C on the last segment
  }
}

void lcd_tm1637_task(void *pvParameter)
{
  bool is_column_on = true;
  lcd = tm1637_init(DISPLAY_CLK, DISPLAY_DTA);

  tm1637_set_brightness(lcd, current_brightness);
  ESP_LOGI(TAG, "Init done");

  check_segments();

  ESP_LOGI(TAG, "Waiting for time");
  xEventGroupWaitBits(global_event_group, IS_NTP_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Got the time");

  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  while (true)
  {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to Amsterdam

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    uint8_t hours = timeinfo.tm_hour;
    uint8_t minutes = timeinfo.tm_min;

    ESP_LOGI(TAG, "Time is %i:%i", hours, minutes);

    if (hours >= 23 || hours <= 7)
    {
      change_brightness_smoothly(MIN_BRIGHTNESS);
    }
    else
    {
      change_brightness_smoothly(MAX_BRIGHTNESS);
    }

    show_time(hours, minutes, is_column_on);
    is_column_on = !is_column_on;

    if (global_is_light_on)
    {
      ESP_LOGI(TAG, "Light is ON");
    }
    else
    {
      ESP_LOGI(TAG, "Light is OFF");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// global_light_sensor_value
// global_inside_temperature_sensor_value
// global_outside_temperature_sensor_value