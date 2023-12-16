#include <esp_http_server.h>
#include <esp_log.h>

#include "global_event_group.h"

static const char *TAG = "Webserver";

extern const unsigned char html_index_html_start[] asm("_binary_index_html_start");
extern const unsigned char html_index_html_end[] asm("_binary_index_html_end");

static esp_err_t send_web_page(httpd_req_t *req)
{
  size_t index_html_size = html_index_html_end - html_index_html_start;
  // +1 for null-terminator
  char *html_page = malloc(index_html_size + 1);

  memcpy(html_page, html_index_html_start, index_html_size);
  html_page[index_html_size] = '\0';

  // Ensure response_data buffer is large enough for the formatted content
  size_t response_data_size = index_html_size + 500;
  char *response_data = malloc(response_data_size);

  // Assuming html_page has format specifiers for the temperatures and light level
  snprintf(response_data, response_data_size, html_page,
           global_inside_temperature,
           global_outside_temperature,
           global_light_level_index + 1,
           LIGHT_LEVELS_AMOUNT,
           global_running_firmware_version);

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

  // ESP_LOGI(TAG, "Waiting for inside temperature");
  // xEventGroupWaitBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  // ESP_LOGI(TAG, "Waiting for outside temperature");
  // xEventGroupWaitBits(global_event_group, IS_OUTSIDE_WEATHER_READING_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  ESP_LOGI(TAG, "Init start");
  httpd_handle_t server = start_webserver();
  ESP_LOGI(TAG, "Init done");

  while (1)
  {
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}