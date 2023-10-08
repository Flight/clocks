#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "../global_event_group.h"

#include "light.h"

static const char *TAG = "HW-072 Light Sensor";

static const gpio_num_t LIGHT_SENSOR_PIN = CONFIG_LIGHT_SENSOR_PIN;
bool global_is_light_on = false;

static void main_isr_handler(void *arg)
{
  xEventGroupSetBits(global_event_group, IS_LIGHT_SENSOR_READING_DONE);
  global_is_light_on = !gpio_get_level(LIGHT_SENSOR_PIN);
}

void light_sensor_task(void *pvParameter)
{
  gpio_reset_pin(LIGHT_SENSOR_PIN);
  gpio_set_direction(LIGHT_SENSOR_PIN, GPIO_MODE_DEF_INPUT);
  gpio_set_pull_mode(LIGHT_SENSOR_PIN, GPIO_PULLUP_PULLDOWN);
  gpio_set_intr_type(LIGHT_SENSOR_PIN, GPIO_INTR_NEGEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(LIGHT_SENSOR_PIN, main_isr_handler, NULL);

  while (1)
  {
    vTaskDelay(portMAX_DELAY);
  }
}