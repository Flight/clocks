#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "sdkconfig.h"

#include "led.h"

static const gpio_num_t LED_GPIO = CONFIG_LED_GPIO;
static bool is_enabled = true;
static const uint8_t INTERVAL_SEC = 2;

static void toggle_led()
{
  gpio_set_level(LED_GPIO, is_enabled);
  is_enabled = !is_enabled;
}

void led_task(void *pvParameter)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

  while (true)
  {
    toggle_led();
    vTaskDelay(1000 * INTERVAL_SEC / portTICK_PERIOD_MS);
  }
}