| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# ESP32 Clocks (ESP-IDF)

It connects to the Wi-Fi and syncs the time with the NTP server, showing the time and temperature on the screen. Optionally saving it to the external timer and showing the temperature from the sensor + from the API. Supports OTA Wi-Fi updates.

The project started from the dead section of the built-in clocks in my bath mirror and moved to my first experience with microcontrollers, C language and creating the home "lab".

![Breadboard](breadboard.jpeg)

![Schematics](schematics.png)
(green wires - data, yellow wires - clock, blue wire - low / high light sensor output)

## Features

- Based on **ESP32** microcontroller (tested with ESP32, ESP32C3 and ESP32S3). I've used the **[WeActStudio ESP32-C3FH4 Core Board](https://github.com/WeActStudio/WeActStudio.ESP32C3CoreBoard)**.

- **TM1637** 7-segment display.
- _Optional:_ **NTP Sync**
- _Optional:_ **DS3231** external timer. Saving time to the separate precise clocks.
- _Optional:_ **SHT3X** environment sensor. Measuring temperature (and optionally humidity).
- _Optional:_ **BMP280/BME280** environment sensor. Measuring temperature (and optionally humidity).
- _Optional:_ **DS1820** temperature sensor. Measuring temperature.
- _Optional:_ **Temperature from API** over Wi-Fi and HTTPS. [api.weatherapi.com](https://www.weatherapi.com)
- _Optional:_ **OTA Updates** over Wi-Fi and HTTPS.
- _Optional:_ **GL5516** Photo resistor. Switching between full and minimal brightness using ADC.
- _Optional:_ **HW-072** Light Sensor. Switching between full and minimal brightness based on the photoresistor output (both analog and digital).

## Requirements

The project was created using [ESP-IDF plugin](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) for Visual Studio Code and [ESP-IDF-LIB](https://esp-idf-lib.readthedocs.io/en/latest/).

## Initial setup

1. Clone the repo `git clone git@github.com:Flight/clocks.git` and go inside: `cd clocks`.
2. Generate the certificate for weatherapi.com

   `echo -n | openssl s_client -connect 192.168.50.100:443 | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' > main/weather_from_api/api_weatherapi_com.pem`

3. Generate a certificate for the OTA Updates. Please **don't forget to fill the server name** on this step:

   **Common Name (e.g. server FQDN or YOUR name) []: 192.168.50.100** (your local IP address).

   `openssl req -newkey rsa:2048 -new -nodes -x509 -days 365 -keyout ota_server/key.pem -out main/ota_update/cert.pem`

4. Change the settings.
   ![Settings](settings.jpeg)

## Run the project

Run the project using the **ESP-IDF Build, Flash and Monitor** button (number 6 on the screenshot).

## OTA Wi-Fi Updates

### Initial Checklist

- **Flash size** according to the settings on the screenshot (> 4 MB is needed for OTA).
- **Partition table** in settings
- **Firmware upgrade endpoint URL** in the Clocks OTA Update section.
- Generated the **certificate** `main/ota_update/cert.pem`.

### Run OTA Update

1. Create new GIT Tag

   `git tag -a v0.0.1 -m "new release"`

   `git push origin v0.0.1` (optional)

2. Run OTA web-server from `ota_server` folder. I'm using this [http-server](https://github.com/http-party/http-server) as **OpenSSL one didn't work properly** and was hanging during download until you don't stop it manually.

   `cd ota_server`

   `sudo npx http-server -S -C ../main/ota_update/cert.pem -p 8070 -c-1`

3. Drop the file `clocks.bin` to `ota_server` folder or setup the build to generate the output file in that folder.

   You can find the `main.bin` in the `build` folder after you built it. Just copy and rename it to `clocks.bin`.

4. Restart the ESP32. It will automatically start update process in 10 seconds after boot.

## Next steps

- ~~Replace the light sensor with the one which supports dead-zone to prevent blinking when the light is near the threshold.~~ (Done)
- ~~Compare OTA versions to prevent cycle update.~~ (Done)
- Replace the Legacy ADC lib with adc_oneshot.h as ESP-IDF suggests.
- Show if it would be rainy on the display.
- Web server with ESP_LOG outputs. The device already supports OTA Wi-Fi updates, so it will make it even easier.
- Check updates from FTP server or Samba directly from the router.
- Rewrite TM1637 lib as it isn't perfect.
- Add the human presense sensor like LD2410 and turn off the screen if there is nobody in the bath to prevent display degradation. Maybe can add some other cool features with it.

## More photos

![Schematics](prebuild.jpeg)
![Installation](installation.jpeg)
![Done](done.jpeg)
![Photoresistor](photoresistor.png)

## Folder contents

The project **clocks** contains the entry point file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both).

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md
```
