idf_component_register(SRC_DIRS "." "system_state" "led" "wifi" "time" "display" "light" "temperature_from_sensor" "temperature_from_api" "ota_update"
                    INCLUDE_DIRS "."
                    REQUIRES "esp32tm1637" "esp_wifi" "esp_http_client" "esp-tls" "nvs_flash" "i2cdev" "bme680" "json" "ds3231" "app_update" "esp_https_ota"
                    EMBED_TXTFILES "temperature_from_api/api_weatherapi_com.pem" "ota_update/cert.pem")
