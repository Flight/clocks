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

#include "system_state/system_state.h"
#include "led/led.h"
#include "light/light.h"
#include "display/display.h"
#include "time/external_timer.h"
#include "wifi/wifi.h"
#include "time/ntp.h"
#include "temperature_from_sensor/temperature_from_sensor.h"
#include "temperature_from_api/temperature_from_api.h"
#include "ota_update/ota_update.h"
#include "auto_restart/auto_restart.h"

EventGroupHandle_t global_event_group;
float global_inside_temperature;

bool global_is_light_on = false;

static const char *TIMEZONE = CONFIG_TIMEZONE;
static const uint8_t DELAY_UNTIL_SYSTEM_STATE_FIRST_PRINT_SECS = 10;

void app_main(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    nvs_flash_erase();
    nvs_flash_init();
  }

  global_event_group = xEventGroupCreate();
  wifi_connect();
  i2cdev_init();

  // Set timezone
  setenv("TZ", TIMEZONE, 1);
  tzset();

  xTaskCreatePinnedToCore(&led_task, "LED", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&light_sensor_task, "Light Sensor", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(&light_sensor_adc_task, "Light Sensor ADC", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&external_timer_task, "External Timer", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ntp_task, "NTP", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&lcd_tm1637_task, "LCD TM1637", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(&temperature_from_sensor_task, "Temp from Sensor", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&temperature_from_api_task, "Temp from API", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ota_update_task, "OTA Update", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(&auto_restart_task, "Auto Restart Task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);

  vTaskDelay(1000 * DELAY_UNTIL_SYSTEM_STATE_FIRST_PRINT_SECS / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(&system_state_task, "System State", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
}
