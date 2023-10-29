idf_component_register(SRC_DIRS "." "led" "wifi" "time" "display" "light" "temperature_from_sensor" "temperature_from_api"
                    INCLUDE_DIRS "."
                    REQUIRES "esp32tm1637" "esp_wifi" "esp_http_client" "nvs_flash" "i2cdev" "bme680" "json" "ds3231"
                    EMBED_TXTFILES "temperature_from_api/api_weatherapi_com.pem")
