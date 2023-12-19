idf_component_register(SRC_DIRS "." "logs_to_buffer" "system_state" "led" "wifi" "time" "display" "light" "temperature_from_sensor" "weather_from_api" "ota_update" "webserver"
                    INCLUDE_DIRS "."
                    REQUIRES "esp32tm1637" "esp_wifi" "esp_http_client" "esp-tls" "nvs_flash" "i2cdev" "sht3x" "json" "ds3231" "app_update" "esp_https_ota" "bmp280" "esp_adc" "ds18x20" "esp_http_server"
                    EMBED_TXTFILES "weather_from_api/api_weatherapi_com.pem" "ota_update/cert.pem" "webserver/index.html")
