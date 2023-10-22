#include <esp_log.h>
#include <esp_http_client.h>
#include <string.h>
#include <cJSON.h>

#include "sdkconfig.h"
#include "global_event_group.h"

#include "temperature_from_api.h"

static const char *TAG = "Weather API";

static const char *WEATHER_API_URL = "https://api.weatherapi.com/v1/";
static const char *WEATHER_API_KEY = CONFIG_WEATHER_API_KEY;
static const char *WEATHER_CITY = CONFIG_WEATHER_CITY;

static const uint8_t REFRESH_INTERVAL_MINS = 5;
static const uint8_t MAX_RETRIES = 3;

float global_outside_temperature;

extern const char api_weatherapi_com_pem_start[] asm("_binary_api_weatherapi_com_pem_start");
extern const char api_weatherapi_com_pem_end[] asm("_binary_api_weatherapi_com_pem_end");

#define MAX_JSON_SIZE 2048
static char json_buffer[MAX_JSON_SIZE];
static int json_buffer_index = 0;

#define TEMPERATURE_ERROR_CODE -1000
static int retry_count = 0;
static float temperature_from_json = TEMPERATURE_ERROR_CODE;

float get_temperature_from_json(char *json_string)
{
  float temperature = TEMPERATURE_ERROR_CODE;

  ESP_LOGI(TAG, "JSON string: %s", json_string);

  cJSON *response_json = cJSON_Parse(json_string);
  if (response_json != NULL)
  {
    cJSON *current = cJSON_GetObjectItem(response_json, "current");
    if (current != NULL)
    {
      cJSON *temperature_json = cJSON_GetObjectItem(current, "temp_c");
      if (temperature_json != NULL && cJSON_IsNumber(temperature_json))
      {
        float temperature = temperature_json->valuedouble;
        return temperature;
      }
      else
      {
        ESP_LOGE(TAG, "Failed to get 'temp_c' or it's not a number");
      }
    }
    else
    {
      ESP_LOGE(TAG, "Failed to get 'current' from JSON response");
    }
    cJSON_Delete(response_json);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to parse JSON response");
  }

  return temperature;
}

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
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
      if (json_buffer_index + evt->data_len < MAX_JSON_SIZE)
      {
        memcpy(json_buffer + json_buffer_index, evt->data, evt->data_len);
        json_buffer_index += evt->data_len;
      }
      else
      {
        ESP_LOGE(TAG, "JSON buffer overflow");
      }
    }
    break;

  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    json_buffer[json_buffer_index] = '\0'; // Null terminate the JSON string
    temperature_from_json = get_temperature_from_json(json_buffer);
    if (temperature_from_json == TEMPERATURE_ERROR_CODE || strlen(json_buffer) == 0)
    {
      ESP_LOGE(TAG, "Can't get temperature");
    }
    else
    {
      global_outside_temperature = temperature_from_json;
      ESP_LOGI(TAG, "Temperature is %f", global_outside_temperature);
      xEventGroupSetBits(global_event_group, IS_OUTSIDE_TEMPERATURE_READING_DONE_BIT);
    }
    json_buffer_index = 0; // Reset for next request
    break;

  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  }

  return ESP_OK;
}

void temperature_from_api_task(void *pvParameter)
{
  char full_url[256];
  sprintf(full_url, "%scurrent.json?key=%s&q=%s&aqi=no", WEATHER_API_URL, WEATHER_API_KEY, WEATHER_CITY);

  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  while (1)
  {
    esp_http_client_config_t config = {
        .url = full_url,
        .event_handler = _http_event_handle,
        .disable_auto_redirect = true,
        .cert_pem = api_weatherapi_com_pem_start,
        .timeout_ms = 20000,
        .buffer_size = 2048};

    esp_http_client_handle_t client = esp_http_client_init(&config);

    while (retry_count < MAX_RETRIES)
    {
      esp_err_t err = esp_http_client_perform(client);

      if (temperature_from_json != TEMPERATURE_ERROR_CODE)
      {
        break;
      }

      ESP_LOGW(TAG, "HTTP GET request failed (attempt %d)", retry_count + 1);
      vTaskDelay(1000 * 10 / portTICK_PERIOD_MS);
      retry_count++;
    }

    esp_http_client_cleanup(client);
    retry_count = 0;
    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}