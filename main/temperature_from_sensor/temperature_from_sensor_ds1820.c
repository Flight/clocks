#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>
#include <ds18x20.h>
#include <inttypes.h>

#include "sdkconfig.h"
#include "global_event_group.h"

#include "temperature_from_sensor.h"

static const gpio_num_t SENSOR_GPIO = CONFIG_ONEWIRE_GPIO;
static const uint8_t REFRESH_INTERVAL_MINS = 1;

static const char *TAG = "DS1820 Temperature Sensor";

uint64_t find_sensor_address()
{
  size_t sensor_count = 0;
  onewire_addr_t addrs[1];

  ds18x20_scan_devices(SENSOR_GPIO, addrs, 1, &sensor_count);

  if (sensor_count > 0)
  {
    ESP_LOGI(TAG, "Sensor address: %lld", addrs[0]);
    return addrs[0];
  }
  else
  {
    ESP_LOGE(TAG, "No sensors found!");
    return 0;
  }
}

void temperature_from_DS1820_sensor_task(void *pvParameter)
{
  // Not needed if there is a pull-up resistor on the data line (4.7k)
  // gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);

  float temperature;
  uint64_t sensor_address = find_sensor_address();

  if (sensor_address == 0)
  {
    ESP_LOGW(TAG, "Sensor not found, cleaning the task");
    vTaskDelete(NULL);
  }

  while (true)
  {
    xEventGroupClearBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);

    if (ds18x20_measure_and_read(SENSOR_GPIO, sensor_address, &temperature) == ESP_OK)
    {
      ESP_LOGI(TAG, "Temperature: %.2f °C", temperature);
      global_inside_temperature = temperature;
      xEventGroupSetBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT);
    }
    else
    {
      ESP_LOGE(TAG, "Can't measure the temperature");
    }

    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}
