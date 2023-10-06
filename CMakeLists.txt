# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

include(FetchContent)
FetchContent_Declare(
  espidflib
  GIT_REPOSITORY https://github.com/UncleRus/esp-idf-lib.git
)
FetchContent_MakeAvailable(espidflib)

FetchContent_Declare(
  bme680espidf
  GIT_REPOSITORY git@github.com:gschorcht/bme680-esp-idf.git
)
FetchContent_MakeAvailable(bme680espidf)

FetchContent_Declare(
 esp32tm1637
 GIT_REPOSITORY git@github.com:petrows/esp-32-tm1637.git
)
FetchContent_Populate(esp32tm1637)
file(COPY ${esp32tm1637_SOURCE_DIR}/ DESTINATION ${CMAKE_SOURCE_DIR}/components/esp32tm1637)

set(EXTRA_COMPONENT_DIRS ${espidflib_SOURCE_DIR}/components ${bme680espidf_SOURCE_DIR}/components ${esp32tm1637_SOURCE_DIR})
project(main)
