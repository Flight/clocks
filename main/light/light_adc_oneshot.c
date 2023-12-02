#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "esp_adc/adc_oneshot.h"
#include <esp_log.h>

#include "sdkconfig.h"
#include "../global_event_group.h"

#include "light.h"

static const char *TAG = "Light Sensor ADC Oneshot";

static const gpio_num_t LIGHT_SENSOR_PIN = CONFIG_LIGHT_SENSOR_PIN;
static const uint16_t ADC_SWITCH_VALUE = 800; // 3550 for MH light sensor breakout board
static const uint16_t ADC_DRIFTING_TOLERANCE = 50;

void light_sensor_adc_oneshot_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Init start");
  bool is_event_bit_set = false;
  bool prev_global_is_light_on = false;
  adc_channel_t channel;
  adc_unit_t unit;

  esp_err_t err = adc_oneshot_io_to_channel(LIGHT_SENSOR_PIN, &unit, &channel);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Pin %d is not ADC pin!", LIGHT_SENSOR_PIN);
    vTaskDelete(NULL);
  }
  else
  {
    ESP_LOGI(TAG, "ADC channel: %d", channel);
  }
  adc_oneshot_unit_handle_t adc_handle;
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = unit,
  };
  adc_oneshot_new_unit(&init_config, &adc_handle);

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_11,
  };
  adc_oneshot_config_channel(adc_handle, channel, &config);
  ESP_LOGI(TAG, "Init end");

  while (1)
  {
    int adc_value;
    adc_oneshot_read(adc_handle, channel, &adc_value);
    // ESP_LOGI(TAG, "ADC value: %d", adc_value);

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