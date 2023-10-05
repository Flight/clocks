#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

#include "sdkconfig.h"
#include "bme680.h"

#include "temperature.h"

#define PORT 0
#define ADDR BME680_I2C_ADDR_1 // Try the BME680_I2C_ADDR_0 if this doesn't work

static const char *TAG = "BME680 Sensor";

static bme680_values_float_t values;
static uint32_t duration;

static const gpio_num_t BME680_SDA_PIN = CONFIG_SDA_PIN;
static const gpio_num_t BME680_SCL_PIN = CONFIG_SCL_PIN;

void temperature_task(void *pvParameter)
{
  bme680_t sensor;
  memset(&sensor, 0, sizeof(bme680_t));

  bme680_init_desc(&sensor, ADDR, PORT, BME680_SDA_PIN, BME680_SCL_PIN);
  bme680_init_sensor(&sensor);

  uint32_t measurement_duration;
  bme680_get_measurement_duration(&sensor, &measurement_duration);

  bme680_values_float_t values;

  while (1)
  {
    if (bme680_force_measurement(&sensor) == ESP_OK)
    {
      // passive waiting until measurement results are available
      vTaskDelay(measurement_duration);

      if (bme680_get_results_float(&sensor, &values) == ESP_OK)
      {
        ESP_LOGI(TAG, "BME680 Sensor: %.2f °C, %.2f %%",
                 values.temperature, values.humidity);
      }
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
