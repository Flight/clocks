#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

#include "sdkconfig.h"
#include "bme680.h"
#include "global_event_group.h"

#include "temperature_from_sensor.h"

#define PORT 0
#define ADDR BME680_I2C_ADDR_1 // Try the BME680_I2C_ADDR_0 if this doesn't work

static const char *TAG = "BME680 Sensor";

static const gpio_num_t SDA_PIN = CONFIG_SDA_PIN;
static const gpio_num_t SCL_PIN = CONFIG_SCL_PIN;

static const uint8_t REFRESH_INTERVAL_MINS = 1;
// BME680 is very unprecise because of the self-heating issue
// so we need to add an offset to the temperature
// This offset is determined by measuring the temperature with a
// precise thermometer and the BME680 at the same time
static const float TEMPERATURE_OFFSET = -3;

void temperature_from_sensor_task(void *pvParameter)
{
  bme680_t sensor;
  memset(&sensor, 0, sizeof(bme680_t));

  bme680_init_desc(&sensor, ADDR, PORT, SDA_PIN, SCL_PIN);
  bme680_init_sensor(&sensor);
  bme680_use_heater_profile(&sensor, BME680_HEATER_NOT_USED);
  bme680_set_ambient_temperature(&sensor, 22);

  uint32_t measurement_duration;
  bme680_get_measurement_duration(&sensor, &measurement_duration);

  bme680_values_float_t values;

  while (true)
  {
    if (bme680_force_measurement(&sensor) == ESP_OK)
    {
      // passive waiting until measurement results are available
      vTaskDelay(measurement_duration);

      if (bme680_get_results_float(&sensor, &values) == ESP_OK)
      {
        ESP_LOGI(TAG, "Raw: %.2f °C, %.2f %%",
                 values.temperature, values.humidity);
        ESP_LOGI(TAG, "Compensated: %.2f °C", values.temperature + TEMPERATURE_OFFSET);
        global_inside_temperature = values.temperature + TEMPERATURE_OFFSET;
        xEventGroupSetBits(global_event_group, IS_PRECISE_INSIDE_TEMPERATURE_READING_DONE_BIT);
        xEventGroupSetBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);
      }
      else
      {
        xEventGroupClearBits(global_event_group, IS_PRECISE_INSIDE_TEMPERATURE_READING_DONE_BIT);
      }
    }
    else
    {
      xEventGroupClearBits(global_event_group, IS_PRECISE_INSIDE_TEMPERATURE_READING_DONE_BIT);
    }

    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}
