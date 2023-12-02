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
#include "wifi/wifi.h"
#include "logs_webserver/logs_webserver.h"
#include "light/light.h"
#include "display/display.h"
#include "time/external_timer.h"
#include "time/ntp.h"
#include "temperature_from_sensor/temperature_from_sensor.h"
#include "weather_from_api/weather_from_api.h"
#include "ota_update/ota_update.h"

EventGroupHandle_t global_event_group;
float global_inside_temperature = TEMPERATURE_ERROR_CODE;

static const char *TIMEZONE = CONFIG_TIMEZONE;

void app_main(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    nvs_flash_erase();
    nvs_flash_init();
  }

  global_event_group = xEventGroupCreate();
  i2cdev_init();

  // Set timezone
  setenv("TZ", TIMEZONE, 1);
  tzset();

  xTaskCreatePinnedToCore(&led_task, "LED", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&wifi_task, "Wi-Fi Keeper", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&logs_webserver_task, "Logs Webserver", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&light_sensor_task, "Light Sensor", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&external_timer_task, "External Timer", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ntp_task, "NTP Sync", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&lcd_tm1637_task, "LCD TM1637", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(&temperature_from_SHT3X_sensor_task, "Temp SHT3X", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(&temperature_from_BMP280_sensor_task, "Temp BMP280", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&temperature_from_DS1820_sensor_task, "Temp DS1820", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&weather_from_api_task, "Weather API", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&ota_update_task, "OTA Update", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(&system_state_task, "System State", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL, 1);
}
