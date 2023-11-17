#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "../global_event_group.h"

#include "light.h"

static const char *TAG = "Light Sensor ADC";

static const gpio_num_t LIGHT_SENSOR_PIN = CONFIG_LIGHT_SENSOR_PIN;
static const u_int16_t ADC_SWITCH_VALUE = 800; // 3550 for MH light sensor breakout board
static const u_int16_t ADC_DRIFTING_TOLERANCE = 50;

void light_sensor_adc_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Init start");
  adc1_config_width(ADC_WIDTH_MAX);
  adc1_config_channel_atten(LIGHT_SENSOR_PIN, ADC_ATTEN_DB_11);

  bool is_event_bit_set = false;
  bool prev_global_is_light_on = false;

  while (1)
  {
    u_int16_t adc_value = adc1_get_raw(LIGHT_SENSOR_PIN);
    if (adc_value < ADC_SWITCH_VALUE - ADC_DRIFTING_TOLERANCE || adc_value > ADC_SWITCH_VALUE + ADC_DRIFTING_TOLERANCE)
    {
      global_is_light_on = adc_value > ADC_SWITCH_VALUE;
    }

    if (!is_event_bit_set)
    {
      ESP_LOGI(TAG, "Light sensor initial reading done");
      xEventGroupSetBits(global_event_group, IS_LIGHT_SENSOR_READING_DONE_BIT);
      is_event_bit_set = true;
    }

    if (prev_global_is_light_on != global_is_light_on)
    {
      ESP_LOGI("Light sensor", "Light is %s", global_is_light_on ? "on" : "off");
      prev_global_is_light_on = global_is_light_on;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}