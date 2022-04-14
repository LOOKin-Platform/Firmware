cmake_minimum_required(VERSION 3.5)

#add_definitions(-DBUILD1GEN)

if(DEFINED ENV{HOMEKIT_PATH})
	set(HOMEKIT_PATH $ENV{HOMEKIT_PATH})
else()
	set(HOMEKIT_PATH ${CMAKE_CURRENT_LIST_DIR}/../Toolchain/esp-homekit-sdk)
endif(DEFINED ENV{HOMEKIT_PATH})

set(EXTRA_COMPONENT_DIRS ${HOMEKIT_PATH} ${HOMEKIT_PATH}/components ${HOMEKIT_PATH}/components/homekit "drivers libs src drivers/Periphery")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)
target_add_binary_data(firmware.elf "partitions_16mb_v2" BINARY)
target_add_binary_data(firmware.elf "partitions_4mb_v2" BINARY)
