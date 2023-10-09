#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
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
static uint8_t SEGMENT_COUNT = 3;
static uint8_t SECONDS_UNTIL_TEMPERATURE_SHOWN = 20;
static uint8_t SECONDS_TO_SHOW_TEMPERATURE = 5;

tm1637_led_t *lcd;

static void show_dashes()
{
  for (uint8_t segment = 0; segment < SEGMENT_COUNT + 1; segment++)
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
    for (uint8_t segment = 0; segment < SEGMENT_COUNT + 1; segment++)
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
  char temperature_string[10];

  // Convert the  float to unsigned string with 2 decimal places
  sprintf(temperature_string, "%.2f", fabs(temperature));
  ESP_LOGI(TAG, "Temp string: %s", temperature_string);

  uint8_t digits_start_segment = 0;

  if (temperature < 0)
  {
    tm1637_set_segment_raw(lcd, 0, 0x40); // Minus sign
    digits_start_segment = 1;             // Adjust the starting segment if negative
  }

  for (uint8_t segment = digits_start_segment, i = 0; segment < SEGMENT_COUNT - digits_start_segment && i < strlen(temperature_string); i++)
  {
    if (temperature_string[i] != '.')
    {
      ESP_LOGI(TAG, "temperature_string[i]: %i", temperature_string[i] - '0');
      ESP_LOGI(TAG, "Segment: %i. Showing digit: %i", segment, temperature_string[i] - '0');
      tm1637_set_segment_number(lcd, segment, temperature_string[i] - '0', temperature_string[i + 1] == '.');
      segment++;
    }
  }

  tm1637_set_segment_raw(lcd, 3, 0xe3); // degree sign
}

void lcd_tm1637_task(void *pvParameter)
{
  bool is_column_on = true;
  u_int8_t seconds_time_shown = 0;
  lcd = tm1637_init(DISPLAY_CLK, DISPLAY_DTA);

  // tm1637_set_brightness(lcd, current_brightness);
  tm1637_set_brightness(lcd, MAX_BRIGHTNESS);
  ESP_LOGI(TAG, "Init done");

  check_segments();

  // ESP_LOGI(TAG, "Waiting for time");
  // xEventGroupWaitBits(global_event_group, IS_NTP_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  // ESP_LOGI(TAG, "Got the time");

  // Set timezone to Amsterdam
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  while (true)
  {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    seconds_time_shown++;

    time(&now);

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    uint8_t hours = timeinfo.tm_hour;
    uint8_t minutes = timeinfo.tm_min;

    if (IS_LIGHT_SENSOR_READING_DONE && global_is_light_on)
    {
      change_brightness_smoothly(MAX_BRIGHTNESS);
    }
    else
    {
      change_brightness_smoothly(MIN_BRIGHTNESS);
    }

    if (IS_INSIDE_TEMPERATURE_READING_DONE && global_inside_temperature && seconds_time_shown > SECONDS_UNTIL_TEMPERATURE_SHOWN)
    {
      show_temperature(global_inside_temperature);
      vTaskDelay(SECONDS_TO_SHOW_TEMPERATURE * 1000 / portTICK_PERIOD_MS);
      seconds_time_shown = 0;

      continue;
    }

    show_time(hours, minutes, is_column_on);
    is_column_on = !is_column_on;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // while (true)
  // {
  //   show_temperature(global_inside_temperature);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(345.678);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(53.445);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(12.349);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(2.435);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(0.445);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(-0.781);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(-1.123);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(-4.353);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(-43.323);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  //   show_temperature(-768.323);
  //   vTaskDelay(20000 / portTICK_PERIOD_MS);
  // }
}