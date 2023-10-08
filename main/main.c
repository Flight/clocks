#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include "sdkconfig.h"
#include "i2cdev.h"

#include "global_event_group.h"

#include "wifi/wifi.h"
#include "led/led.h"
#include "display/display.h"
#include "temperature/temperature.h"
#include "time/ntp.h"

EventGroupHandle_t global_event_group;

void app_main(void)
{
  nvs_flash_init();
  global_event_group = xEventGroupCreate();
  wifi_connect();
  i2cdev_init();

  xTaskCreatePinnedToCore(&led_task, "led_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ntp_task, "ntp_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&lcd_tm1637_task, "lcd_tm1637_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(temperature_task, "temperature_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
}
