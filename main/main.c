#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <time.h>

#include "sdkconfig.h"
#include "i2cdev.h"

#include "global_event_group.h"

#include "led/led.h"
#include "light/light.h"
#include "display/display.h"
#include "time/external_timer.h"
#include "wifi/wifi.h"
#include "time/ntp.h"
#include "temperature_from_sensor/temperature_from_sensor.h"
#include "temperature_from_api/temperature_from_api.h"

static const char *TIMEZONE = CONFIG_TIMEZONE;

EventGroupHandle_t global_event_group;
float global_inside_temperature;

void app_main(void)
{
  nvs_flash_init();
  global_event_group = xEventGroupCreate();
  wifi_connect();
  i2cdev_init();

  // Set timezone
  setenv("TZ", TIMEZONE, 1);
  tzset();

  xTaskCreatePinnedToCore(&led_task, "led_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&light_sensor_task, "light_sensor_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&external_timer_task, "external_timer_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ntp_task, "ntp_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&lcd_tm1637_task, "lcd_tm1637_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&temperature_from_sensor_task, "temperature_from_sensor_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&temperature_from_api_task, "temperature_from_api_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
}
