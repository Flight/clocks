#include <esp_http_server.h>
#include <esp_log.h>

#include "global_event_group.h"

static const char *TAG = "Logs Webserver";

static char html_page[] = "<!DOCTYPE HTML><html>\n"
                          "<head>\n"
                          "  <title>ESP-IDF Web Server</title>\n"
                          "  <meta http-equiv=\"refresh\" content=\"10\">\n"
                          "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                          "  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">\n"
                          "  <link rel=\"icon\" href=\"data:,\">\n"
                          "  <style>\n"
                          "    html {font-family: Arial; display: inline-block; text-align: center;}\n"
                          "    p {  font-size: 1.2rem;}\n"
                          "    body {  margin: 0;}\n"
                          "    .topnav { overflow: hidden; background-color: #241d4b; color: white; font-size: 1.7rem; }\n"
                          "    .content { padding: 20px; }\n"
                          "    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }\n"
                          "    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }\n"
                          "    .reading { font-size: 2.8rem; }\n"
                          "    .card.temperature { color: #0e7c7b; }\n"
                          "  </style>\n"
                          "</head>\n"
                          "<body>\n"
                          "  <div class=\"topnav\">\n"
                          "    <h3>ESP-IDF WEB SERVER</h3>\n"
                          "  </div>\n"
                          "  <div class=\"content\">\n"
                          "    <div class=\"cards\">\n"
                          "      <div class=\"card temperature\">\n"
                          "        <h4><i class=\"fas fa-thermometer-half\"></i> INSIDE TEMPERATURE</h4><p><span class=\"reading\">%.2f&deg;C</span></p>\n"
                          "      </div>\n"
                          "      <div class=\"card temperature\">\n"
                          "        <h4><i class=\"fas fa-thermometer-half\"></i> OUTSIDE TEMPERATURE</h4><p><span class=\"reading\">%.2f&deg;C</span></p>\n"
                          "      </div>\n"
                          "      <div class=\"card temperature\">\n"
                          "        <h4><i class=\"fas fa-lightbulb-o\"></i> LIGHT LEVEL</h4><p><span class=\"reading\">%d / %d</span></p>\n"
                          "      </div>\n"
                          "    </div>\n"
                          "  </div>\n"
                          "</body>\n"
                          "</html>";

static esp_err_t send_web_page(httpd_req_t *req)
{
  int response;
  char response_data[sizeof(html_page) + 50];
  memset(response_data, 0, sizeof(response_data));
  sprintf(response_data, html_page, global_inside_temperature, global_outside_temperature, global_light_level_index + 1, LIGHT_LEVELS_AMOUNT);
  response = httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);

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

static httpd_handle_t start_logs_webserver(void)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK)
  {
    httpd_register_uri_handler(server, &uri_get);
  }

  return server;
}

void logs_webserver_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Waiting for inside temperature");
  xEventGroupWaitBits(global_event_group, IS_INSIDE_TEMPERATURE_READING_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Waiting for Wi-Fi");
  xEventGroupWaitBits(global_event_group, IS_WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "Waiting for outside temperature");
  xEventGroupWaitBits(global_event_group, IS_OUTSIDE_WEATHER_READING_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  httpd_handle_t server = start_logs_webserver();

  while (1)
  {
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}