#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

#include "sdkconfig.h"
#include "bmp280.h"
#include "global_event_group.h"

#include "temperature_from_sensor.h"

#define PORT 0
#define ADDR CONFIG_SHT3X_ADDR

static const char *TAG = "BMP280 Sensor";

static const gpio_num_t SDA_PIN = CONFIG_SDA_PIN;
static const gpio_num_t SCL_PIN = CONFIG_SCL_PIN;

static const uint8_t REFRESH_INTERVAL_MINS = 1;

void temperature_from_BMP280_sensor_task(void *pvParameter)
{
  float pressure;
  float temperature;
  float humidity;

  ESP_LOGI(TAG, "Init start");
  bmp280_params_t params;
  bmp280_init_default_params(&params);
  bmp280_t sensor;
  memset(&sensor, 0, sizeof(bmp280_t));
  bmp280_init_desc(&sensor, BMP280_I2C_ADDRESS_0, 0, SDA_PIN, SCL_PIN);
  bmp280_init(&sensor, &params);

  bool bme280p = sensor.id == BME280_CHIP_ID;
  ESP_LOGI(TAG, "BMP280: found %s", bme280p ? "BME280" : "BMP280");
  ESP_LOGI(TAG, "Init end");

  while (true)
  {
    xEventGroupClearBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);
    if (bmp280_read_float(&sensor, &temperature, &pressure, &humidity) == ESP_OK)
    {
      ESP_LOGI(TAG, "Pressure: %.2f Pa, Temperature: %.2f C", pressure, temperature);
      if (bme280p)
      {
        ESP_LOGI(TAG, "Humidity: %.2f\n", humidity);
      }
      global_inside_temperature = temperature;
      xEventGroupSetBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);
      vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGE(TAG, "Can't measure the temperature");
    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}
