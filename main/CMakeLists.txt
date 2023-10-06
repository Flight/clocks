idf_component_register(SRCS "main.c" "ntp.c" "wifi.c" "led.c" "display.c" "temperature.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp32tm1637" "esp_wifi" "nvs_flash" "i2cdev" "bme680")
