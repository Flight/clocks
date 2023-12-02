#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "esp_adc/adc_oneshot.h"
#include <esp_log.h>

#include "sdkconfig.h"
#include "../global_event_group.h"

#include "light.h"

// As ESP32 ADC is not very stable, we use moving average to smooth the readings
#define MOVING_AVERAGE_SIZE 10

uint8_t global_light_level_index = LIGHT_LEVEL_LOW;

static const char *TAG = "Light Sensor";

static const gpio_num_t LIGHT_SENSOR_PIN = CONFIG_LIGHT_SENSOR_PIN;

static const uint16_t ADC_LOW_THRESHOLD = 600;
static const uint16_t ADC_HIGH_THRESHOLD = 1500;
static const uint16_t HYSTERESIS_MARGIN = 50;

void update_light_level(int adc_value)
{
  // ESP_LOGI(TAG, "ADC average value: %d", adc_value);
  switch (global_light_level_index)
  {
  case LIGHT_LEVEL_LOW:
    if (adc_value >= ADC_HIGH_THRESHOLD + HYSTERESIS_MARGIN)
    {
      global_light_level_index = LIGHT_LEVEL_HIGH;
    }
    else if (adc_value >= ADC_LOW_THRESHOLD + HYSTERESIS_MARGIN)
    {
      global_light_level_index = LIGHT_LEVEL_MEDIUM;
    }
    break;

  case LIGHT_LEVEL_MEDIUM:
    if (adc_value < ADC_LOW_THRESHOLD)
    {
      global_light_level_index = LIGHT_LEVEL_LOW;
    }
    else if (adc_value >= ADC_HIGH_THRESHOLD + HYSTERESIS_MARGIN)
    {
      global_light_level_index = LIGHT_LEVEL_HIGH;
    }
    break;

  case LIGHT_LEVEL_HIGH:
    if (adc_value < ADC_LOW_THRESHOLD)
    {
      global_light_level_index = LIGHT_LEVEL_LOW;
    }
    else if (adc_value < ADC_HIGH_THRESHOLD)
    {
      global_light_level_index = LIGHT_LEVEL_MEDIUM;
    }
    break;
  }
}

void light_sensor_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Init start");
  bool is_event_bit_set = false;
  uint8_t prev_global_light_level_index = 0;
  adc_channel_t channel;
  adc_unit_t unit;
  int adc_values[MOVING_AVERAGE_SIZE] = {0}; // Buffer to store recent readings
  int adc_index = 0;                         // Current index for the buffer
  int adc_sum = 0;                           // Sum of the buffer values

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

    // Update buffer
    adc_sum -= adc_values[adc_index];  // Subtract the old value
    adc_values[adc_index] = adc_value; // Add the new value
    adc_sum += adc_value;              // Update the sum

    // Update index
    adc_index = (adc_index + 1) % MOVING_AVERAGE_SIZE;

    // Calculate moving average
    int adc_average = adc_sum / MOVING_AVERAGE_SIZE;

    // Use the average value for light level decision
    update_light_level(adc_average);

    if (!is_event_bit_set)
    {
      ESP_LOGI(TAG, "Light sensor initial reading done");
      xEventGroupSetBits(global_event_group, IS_LIGHT_SENSOR_READING_DONE_BIT);
      is_event_bit_set = true;
    }

    if (prev_global_light_level_index != global_light_level_index)
    {
      ESP_LOGI("Light sensor", "Light level is %d out of %d (prev was %d)", global_light_level_index + 1, LIGHT_LEVELS_AMOUNT, prev_global_light_level_index + 1);
      prev_global_light_level_index = global_light_level_index;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}