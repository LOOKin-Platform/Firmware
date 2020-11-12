cmake_minimum_required(VERSION 3.5)

#if(CONFIG_HAP_MFI_HW_AUTH)
#	 add_definitions(-DHAVE_MFI_HW_AUTH=1)
#endif()

#add_definitions(-DPLATFORMS="IP")


#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT)
	if(DEFINED ENV{HOMEKIT_PATH})
  		set(HOMEKIT_PATH $ENV{HOMEKIT_PATH})
	else()
  		set(HOMEKIT_PATH ${CMAKE_CURRENT_LIST_DIR}/../Toolchain/esp-homekit-sdk)
	endif(DEFINED ENV{HOMEKIT_PATH})

	set(EXTRA_COMPONENT_DIRS ${HOMEKIT_PATH} ${HOMEKIT_PATH}/components ${HOMEKIT_PATH}/components/homekit "drivers libs src")
	

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)