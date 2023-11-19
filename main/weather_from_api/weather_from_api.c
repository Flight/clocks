#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include <string.h>
#include <cJSON.h>

#include "sdkconfig.h"
#include "global_event_group.h"

#include "weather_from_api.h"

float global_outside_temperature;
int8_t global_outside_will_it_rain;

static const char *TAG = "Weather API";

static const char *WEATHER_API_URL = "https://api.weatherapi.com/v1/";
static const char *WEATHER_API_KEY = CONFIG_WEATHER_API_KEY;
static const char *WEATHER_CITY = CONFIG_WEATHER_CITY;

static const uint8_t REFRESH_INTERVAL_MINS = 5;
static const uint8_t RETRY_INTERVAL_SECS = 10;
static const uint8_t MAXIMUM_RETRY = 3;

extern const char api_weatherapi_com_pem_start[] asm("_binary_api_weatherapi_com_pem_start");
extern const char api_weatherapi_com_pem_end[] asm("_binary_api_weatherapi_com_pem_end");

#define MAX_HTTP_OUTPUT_BUFFER 2048
#define TEMPERATURE_ERROR_CODE -1000
#define WILL_IT_RAIN_ERROR_CODE -1

static uint8_t retry_count = 0;
static float temperature_from_json = TEMPERATURE_ERROR_CODE;
static int8_t will_it_rain_from_json = WILL_IT_RAIN_ERROR_CODE;

static void get_data_from_json(char *json_string)
{
  temperature_from_json = TEMPERATURE_ERROR_CODE;
  will_it_rain_from_json = WILL_IT_RAIN_ERROR_CODE;

  ESP_LOGI(TAG, "JSON string: %s", json_string);

  cJSON *response_json = cJSON_Parse(json_string);
  if (response_json == NULL)
  {
    ESP_LOGE(TAG, "Failed to parse JSON response");
    return;
  }

  cJSON *current = cJSON_GetObjectItem(response_json, "current");
  if (current != NULL)
  {
    cJSON *temperature_json = cJSON_GetObjectItem(current, "temp_c");
    if (temperature_json != NULL && cJSON_IsNumber(temperature_json))
    {
      temperature_from_json = temperature_json->valuedouble;
    }
  }

  // Extracting will_it_rain from JSON
  cJSON *forecast = cJSON_GetObjectItem(response_json, "forecast");
  if (forecast != NULL)
  {
    cJSON *forecastday = cJSON_GetObjectItem(forecast, "forecastday");
    if (forecastday != NULL)
    {
      cJSON *firstDay = cJSON_GetArrayItem(forecastday, 0);
      if (firstDay != NULL)
      {
        cJSON *day = cJSON_GetObjectItem(firstDay, "day");
        if (day != NULL)
        {
          cJSON *will_it_rain = cJSON_GetObjectItem(day, "daily_will_it_rain");
          if (will_it_rain != NULL && cJSON_IsNumber(will_it_rain))
          {
            will_it_rain_from_json = will_it_rain->valueint;
          }
        }
      }
    }
  }

  cJSON_Delete(response_json);
}

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  static char *output_buffer = NULL; // Buffer to store response of http request from event handler
  static uint32_t output_length = 0; // Stores number of bytes read

  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
    break;

  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;

  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;

  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;

  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

    if (esp_http_client_is_chunked_response(evt->client))
    {
      ESP_LOGI(TAG, "The data is chunked");
      // Reallocate buffer for chunked response
      char *temp_buffer = realloc(output_buffer, output_length + evt->data_len + 1);
      if (!temp_buffer)
      {
        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
        if (output_buffer)
        {
          free(output_buffer);
          output_buffer = NULL;
          output_length = 0;
        }
        return ESP_FAIL;
      }
      output_buffer = temp_buffer;
    }
    else if (output_buffer == NULL)
    {
      ESP_LOGI(TAG, "The data is not chunked");
      // Allocate buffer for non-chunked response
      uint32_t content_length = esp_http_client_get_content_length(evt->client);
      output_buffer = (char *)calloc(content_length + 1, sizeof(char));
      if (!output_buffer)
      {
        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
        return ESP_FAIL;
      }
    }

    // Copy data to buffer
    if (evt->data_len)
    {
      uint32_t copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_length));
      memcpy(output_buffer + output_length, evt->data, copy_len);
      output_length += copy_len;
    }
    break;

  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    if (output_buffer != NULL)
    {
      get_data_from_json(output_buffer);
      if (temperature_from_json == TEMPERATURE_ERROR_CODE || strlen(output_buffer) == 0)
      {
        ESP_LOGE(TAG, "Can't get temperature");
      }
      else
      {
        global_outside_temperature = temperature_from_json;
        ESP_LOGI(TAG, "Temperature is %f", global_outside_temperature);
        if (will_it_rain_from_json != WILL_IT_RAIN_ERROR_CODE)
        {
          global_outside_will_it_rain = will_it_rain_from_json;
          ESP_LOGI(TAG, "Will it rain today? %s", will_it_rain_from_json ? "Yes" : "No");
        }
        xEventGroupSetBits(global_event_group, IS_OUTSIDE_WEATHER_READING_DONE_BIT);
      }

      free(output_buffer);
      output_buffer = NULL;
      output_length = 0;
    }
    else
    {
      ESP_LOGE(TAG, "Output buffer is 0!");
    }
    break;

  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    int mbedtls_err = 0;
    esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
    if (err != 0)
    {
      ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
      ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
    }
    if (output_buffer != NULL)
    {
      free(output_buffer);
      output_buffer = NULL;
      output_length = 0;
    }
    break;

  case HTTP_EVENT_REDIRECT:
    ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }

  return ESP_OK;
}

void weather_from_api_task(void *pvParameter)
{
  char full_url[256];
  sprintf(full_url, "%sforecast.json?key=%s&q=%s", WEATHER_API_URL, WEATHER_API_KEY, WEATHER_CITY);

  esp_http_client_config_t config = {
      .url = full_url,
      .event_handler = _http_event_handler,
      .cert_pem = api_weatherapi_com_pem_start,
      .timeout_ms = 20000,
      .buffer_size = 4096};

  while (true)
  {
    xEventGroupClearBits(global_event_group, IS_OUTSIDE_WEATHER_READING_DONE_BIT);

    ESP_LOGI(TAG, "Waiting for Wi-Fi");
    xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (retry_count < MAXIMUM_RETRY)
    {
      esp_http_client_perform(client);

      if (temperature_from_json == TEMPERATURE_ERROR_CODE)
      {
        ESP_LOGW(TAG, "Failed to get the weather (attempt %d of %d)", retry_count + 1, MAXIMUM_RETRY);
        vTaskDelay(1000 * RETRY_INTERVAL_SECS / portTICK_PERIOD_MS);
        retry_count++;
        continue;
      }
    }
    else
    {
      ESP_LOGE(TAG, "Can't fetch the weather. Next retry in %d minutes.", REFRESH_INTERVAL_MINS);
    }

    esp_http_client_cleanup(client);
    retry_count = 0;
    vTaskDelay(1000 * 60 * REFRESH_INTERVAL_MINS / portTICK_PERIOD_MS);
  }
}