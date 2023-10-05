#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include "sdkconfig.h"
#include "i2cdev.h"

#include "wifi.h"
#include "led.h"
// #include "display.h"
#include "temperature.h"
#include "ntp.h"
#include "ntp_event.h"

#define NTP_EVENT_BIT BIT0

EventGroupHandle_t ntp_event_group;

void background_task(void *pvParameter)
{
  while (true)
  {
    printf("This is the background task, which will run once a 5 seconds.\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void app_main(void)
{
  nvs_flash_init();
  ntp_event_group = xEventGroupCreate();
  wifi_connect();
  ESP_ERROR_CHECK(i2cdev_init());

  xTaskCreatePinnedToCore(&led_task, "led_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ntp_task, "ntp_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(&lcd_tm1637_task, "lcd_tm1637_task", 8096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(temperature_task, "temperature_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL, 1);
}
