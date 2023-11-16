#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

#include "sdkconfig.h"
#include "sht3x.h"
#include "global_event_group.h"

#include "temperature_from_sensor.h"

#define PORT 0
#define ADDR CONFIG_SHT3X_ADDR

static const char *TAG = "SHT3X Sensor";

static const gpio_num_t SDA_PIN = CONFIG_SDA_PIN;
static const gpio_num_t SCL_PIN = CONFIG_SCL_PIN;

static const uint8_t REFRESH_INTERVAL_MINS = 1;

void temperature_from_sensor_task(void *pvParameter)
{
  float temperature;
  float humidity;

  sht3x_t sensor;
  memset(&sensor, 0, sizeof(sht3x_t));

  ESP_LOGI(TAG, "Init start");
  sht3x_init_desc(&sensor, ADDR, PORT, SDA_PIN, SCL_PIN);
  sht3x_init(&sensor);
  uint8_t measurement_duration = sht3x_get_measurement_duration(SHT3X_HIGH);
  ESP_LOGI(TAG, "Init end");

  while (true)
  {
    xEventGroupClearBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);

    sht3x_start_measurement(&sensor, SHT3X_SINGLE_SHOT, SHT3X_HIGH);

    vTaskDelay(measurement_duration);

    if (sht3x_get_results(&sensor, &temperature, &humidity) == ESP_OK)
    {
      ESP_LOGI(TAG, "Temperature: %.2f °C, %.2f %%\n", temperature, humidity);
      global_inside_temperature = temperature;
      xEventGroupSetBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);
      vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGE(TAG, "Can't measure the temperature");
    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}
