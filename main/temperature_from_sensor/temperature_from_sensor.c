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

static const gpio_num_t BME680_SDA_PIN = CONFIG_SDA_PIN;
static const gpio_num_t BME680_SCL_PIN = CONFIG_SCL_PIN;

static const uint8_t REFRESH_INTERVAL_MINS = 1;

void temperature_from_sensor_task(void *pvParameter)
{
  bme680_t sensor;
  memset(&sensor, 0, sizeof(bme680_t));

  bme680_init_desc(&sensor, ADDR, PORT, BME680_SDA_PIN, BME680_SCL_PIN);
  bme680_init_sensor(&sensor);

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
        ESP_LOGI(TAG, "%.2f °C, %.2f %%",
                 values.temperature, values.humidity);
        global_inside_temperature = values.temperature;
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
