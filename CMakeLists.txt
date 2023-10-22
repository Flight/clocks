# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

include(FetchContent)
FetchContent_Declare(
  espidflib
  GIT_REPOSITORY git@github.com:UncleRus/esp-idf-lib.git
  GIT_TAG e8fff9a2d638ce08716b69fa66382bbfa0bfffdf
)
FetchContent_MakeAvailable(espidflib)

FetchContent_Declare(
 esp32tm1637
 GIT_REPOSITORY git@github.com:petrows/esp-32-tm1637.git
 GIT_REF_COMMIT cf4bfbda57c66cfe953320cb41dc5e0766ea8f64
)
FetchContent_Populate(esp32tm1637)
file(COPY ${esp32tm1637_SOURCE_DIR}/ DESTINATION ${CMAKE_SOURCE_DIR}/components/esp32tm1637)

set(EXTRA_COMPONENT_DIRS ${espidflib_SOURCE_DIR}/components)
project(main)
