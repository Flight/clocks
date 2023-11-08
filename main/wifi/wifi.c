#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include "global_event_group.h"

#include "wifi.h"

#define SSID CONFIG_WIFI_SSID
#define PASSWORD CONFIG_WIFI_PASSWORD

static const char *TAG = "Wi-Fi";

static const uint32_t MAXIMUM_RETRY = CONFIG_WIFI_MAXIMUM_RETRY;
static const uint8_t DELAY_UNTIL_NEXT_RETRY_SERIES_SECS = 60;

static uint32_t retry_num = 0;
static bool is_manually_disconnected = false;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch (event_id)
  {
  case WIFI_EVENT_STA_START:
    ESP_LOGI(TAG, "Connecting to SSID: %s, password: %.2s...", SSID, PASSWORD);
    break;

  case WIFI_EVENT_STA_CONNECTED:
    ESP_LOGI(TAG, "Connected to SSID: %s", SSID);
    break;

  case WIFI_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "Lost connection.");
    xEventGroupClearBits(global_event_group, IS_WIFI_CONNECTED_BIT);
    if (retry_num < MAXIMUM_RETRY)
    {
      esp_wifi_connect();
      retry_num++;
      ESP_LOGI(TAG, "Retrying to connect to SSID: %s, password: %.2s... (%lu)", SSID, PASSWORD, retry_num);
    }
    else
    {
      ESP_LOGI(TAG, "Failed to connect to SSID: %s, password: %.2s", SSID, PASSWORD);
      vTaskDelay(1000 * DELAY_UNTIL_NEXT_RETRY_SERIES_SECS / portTICK_PERIOD_MS);
      retry_num = 0;
    }
    break;

  case IP_EVENT_STA_GOT_IP:
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(global_event_group, IS_WIFI_CONNECTED_BIT);
    break;

  default:
    break;
  }
}

void wifi_connect()
{
  is_manually_disconnected = false;

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_initiation);
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
  wifi_config_t wifi_configuration = {
      .sta = {
          .ssid = SSID,
          .password = PASSWORD,
      }};

  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
  esp_wifi_start();
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_connect();
}

void wifi_disconnect()
{
  is_manually_disconnected = true;

  esp_wifi_disconnect();
}