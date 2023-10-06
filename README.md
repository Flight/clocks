| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# _Clocks_

The basic version of the clocks project, which is using:

- ESP32 microcontroller
- TM1637 7-segment display
- BME680 environment sensor (temperature)

It connects to the Wi-Fi and syncs the time with the NTP server, showing the time and temperature on the screen.

The project was created using [ESP-IDF plugin](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) for Visual Studio Code and [ESP-IDF-LIB](https://esp-idf-lib.readthedocs.io/en/latest/).

## Folder contents

The project **clocks-basic** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

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
