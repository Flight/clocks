#include <esp_http_server.h>
#include <esp_log.h>

#include "global_event_group.h"

static const char *TAG = "Webserver";

extern const unsigned char html_index_html_start[] asm("_binary_index_html_start");
extern const unsigned char html_index_html_end[] asm("_binary_index_html_end");

static void replace_placeholder(char *buffer, size_t buffer_size, const char *placeholder, const char *replacement)
{
  char *pos = strstr(buffer, placeholder);
  if (pos)
  {
    size_t len_before = pos - buffer;
    size_t len_placeholder = strlen(placeholder);
    size_t len_replacement = strlen(replacement);
    size_t len_after = strlen(pos + len_placeholder);

    if (len_before + len_replacement + len_after < buffer_size)
    {
      memmove(pos + len_replacement, pos + len_placeholder, len_after + 1); // +1 for null terminator
      memcpy(pos, replacement, len_replacement);
    }
  }
}

static esp_err_t send_web_page(httpd_req_t *req)
{
  size_t index_html_size = html_index_html_end - html_index_html_start;
  char *html_page = malloc(index_html_size + 1);
  if (!html_page)
  {
    // Handle malloc failure
    return ESP_ERR_NO_MEM;
  }

  memcpy(html_page, html_index_html_start, index_html_size);
  html_page[index_html_size] = '\0';

  // Ensure response_data buffer is large enough for the formatted content
  size_t response_data_size = index_html_size + 500 + LOG_BUFFER_SIZE;
  char *response_data = malloc(response_data_size);
  if (!response_data)
  {
    // Handle malloc failure
    free(html_page);
    return ESP_ERR_NO_MEM;
  }

  char inside_temperature_string[20];
  if (global_inside_temperature == TEMPERATURE_ERROR_CODE)
  {
    strcpy(inside_temperature_string, "N/A");
  }
  else
  {
    sprintf(inside_temperature_string, "%.2f°C", global_inside_temperature);
  }

  char outside_temperature_string[20];
  if (global_outside_temperature == TEMPERATURE_ERROR_CODE)
  {
    strcpy(outside_temperature_string, "N/A");
  }
  else
  {
    sprintf(outside_temperature_string, "%.2f°C", global_outside_temperature);
  }

  // Convert integer values to strings before using them in replace_placeholder
  char light_level_string[20];
  sprintf(light_level_string, "%d", global_light_level_index + 1);
  char light_levels_amount_string[20];
  sprintf(light_levels_amount_string, "%d", LIGHT_LEVELS_AMOUNT);

  strcpy(response_data, html_page);

  replace_placeholder(response_data, response_data_size, "{INSIDE_TEMPERATURE}", inside_temperature_string);
  replace_placeholder(response_data, response_data_size, "{OUTSIDE_TEMPERATURE}", outside_temperature_string);
  replace_placeholder(response_data, response_data_size, "{LIGHT_LEVEL}", light_level_string);
  replace_placeholder(response_data, response_data_size, "{LIGHT_LEVELS_AMOUNT}", light_levels_amount_string);
  replace_placeholder(response_data, response_data_size, "{FIRMWARE_VERSION}", global_running_firmware_version);
  replace_placeholder(response_data, response_data_size, "{LOGS}", global_log_buffer);

  esp_err_t response = httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);

  free(html_page);
  free(response_data);

  return response;
}

static esp_err_t get_req_handler(httpd_req_t *req)
{
  return send_web_page(req);
}

static httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

static httpd_handle_t start_webserver(void)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK)
  {
    httpd_register_uri_handler(server, &uri_get);
  }

  return server;
}

void webserver_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  ESP_LOGI(TAG, "Init start");
  start_webserver();
  ESP_LOGI(TAG, "Init done");

  while (1)
  {
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}