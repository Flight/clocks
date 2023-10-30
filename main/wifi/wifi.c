#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include "global_event_group.h"

#include "wifi.h"

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD
#define WIFI_MAXIMUM_RETRY CONFIG_WIFI_MAXIMUM_RETRY

static const char *TAG = "Wi-Fi";
static uint8_t retry_num = 0;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_id == WIFI_EVENT_STA_START)
  {
    ESP_LOGI(TAG, "Connecting to SSID: %s, password: %.2s...", WIFI_SSID, WIFI_PASS);
  }
  else if (event_id == WIFI_EVENT_STA_CONNECTED)
  {
    ESP_LOGI(TAG, "Connected to SSID: %s", WIFI_SSID);
  }
  else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    ESP_LOGI(TAG, "Lost connection.");
    if (retry_num < WIFI_MAXIMUM_RETRY)
    {
      esp_wifi_connect();
      retry_num++;
      ESP_LOGI(TAG, "Retrying to connect SSID: %s, password: %.2s... (%i)", WIFI_SSID, WIFI_PASS, retry_num);
    }
    else
    {
      ESP_LOGI(TAG, "Failed to connect to SSID: %s, password: %.2s", WIFI_SSID, WIFI_PASS);
    }
  }
  else if (event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(global_event_group, IS_WIFI_CONNECTED_BIT);
  }
}

void wifi_connect()
{
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_initiation);
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
  wifi_config_t wifi_configuration = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASS,
      }};

  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
  esp_wifi_start();
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_connect();
}

void wifi_disconnect()
{
  esp_wifi_disconnect();
}