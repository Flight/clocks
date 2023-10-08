idf_component_register(SRC_DIRS "." "led" "wifi" "time" "display" "temperature" "light"
                    INCLUDE_DIRS "."
                    REQUIRES "esp32tm1637" "esp_wifi" "nvs_flash" "i2cdev" "bme680")
