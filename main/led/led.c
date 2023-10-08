#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "led.h"
#include "sdkconfig.h"

static const gpio_num_t LED_GPIO = CONFIG_LED_GPIO;
static bool is_enabled = true;

static void toggle_led()
{
  gpio_set_level(LED_GPIO, is_enabled);
  is_enabled = !is_enabled;
}

void led_task(void *pvParameter)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  while (1)
  {
    toggle_led();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}