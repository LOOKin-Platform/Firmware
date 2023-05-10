cmake_minimum_required(VERSION 3.5)

#add_definitions(-DBUILD1GEN)

add_definitions(-DMATTER_WIFI_OVERRIDE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party/connectedhomeip/examples/common/cmake/idf_flashing.cmake)

set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/third_party/connectedhomeip/config/esp32/components"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/connectedhomeip/examples/common/QRCode"
    "drivers"
    "libs"
    "main"
    "drivers/Periphery")

project(firmware)

idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-DCHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT=16;" APPEND) 
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format" APPEND)

target_add_binary_data(firmware.elf "partitions_16mb_v2" BINARY)
target_add_binary_data(firmware.elf "partitions_4mb_v2" BINARY)

flashing_script()
