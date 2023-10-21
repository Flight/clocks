#include <esp_log.h>
#include <esp_http_client.h>
#include <string.h>
#include <cJSON.h>

#include "sdkconfig.h"
#include "global_event_group.h"

#include "temperature_from_api.h"

#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char *TAG = "Weather API";

static const char *WEATHER_API_URL = "https://api.weatherapi.com/v1/";
static const char *WEATHER_API_KEY = CONFIG_WEATHER_API_KEY;
static const char *WEATHER_CITY = CONFIG_WEATHER_CITY;

static const uint8_t REFRESH_INTERVAL_MINS = 30;

float global_inside_temperature;
float global_outside_temperature;

extern const char api_weatherapi_com_pem_start[] asm("_binary_api_weatherapi_com_pem_start");
extern const char api_weatherapi_com_pem_end[] asm("_binary_api_weatherapi_com_pem_end");

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
    printf("%.*s", evt->data_len, (char *)evt->data);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (!esp_http_client_is_chunked_response(evt->client))
    {
      printf("%.*s", evt->data_len, (char *)evt->data);
    }

    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  }
  return ESP_OK;
}

void temperature_from_api_task(void *pvParameter)
{
  ESP_LOGI(TAG, "temperature_from_api_task started!");
  // Declare local_response_buffer with size (MAX_HTTP_OUTPUT_BUFFER + 1) to prevent out of bound access when
  // it is used by functions like strlen(). The buffer should only be used upto size MAX_HTTP_OUTPUT_BUFFER
  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
  char full_url[256];
  sprintf(full_url, "%scurrent.json?key=%s&q=%s&aqi=no", WEATHER_API_URL, WEATHER_API_KEY, WEATHER_CITY);

  esp_http_client_config_t config = {
      .url = full_url,
      .event_handler = _http_event_handle,
      .user_data = local_response_buffer,
      .disable_auto_redirect = true,
      .cert_pem = api_weatherapi_com_pem_start};

  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  while (1)
  {
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
      // ESP_LOGI(TAG, "Status = %x, content_length = %x",
      //          esp_http_client_get_status_code(client),
      //          esp_http_client_get_content_length(client));
      ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));
      // cJSON *response_json = cJSON_Parse(local_response_buffer);
      // cJSON *current = cJSON_GetObjectItem(response_json, "current");
      // float temperature = cJSON_GetObjectItem(current, "temp_c")->valuedouble;
      // ESP_LOGI(TAG, "temperature is %f", temperature);
    }
    else
    {
      ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}