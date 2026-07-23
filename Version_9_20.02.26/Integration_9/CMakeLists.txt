# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Integration_9)

#Added below line coz usb functionaling was throwing some error. This flashes the data folder to littlefs filesystem
#littlefs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/data FLASH_IN_PROJECT)