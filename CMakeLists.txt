# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#set(EXTRA_COMPONENT_DIRS $ENV{HOMEKITADK_PATH} "drivers libs src")

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

	#if(DEFINED ENV{IDF_PATH})
  	#	set(IDF_PATH $ENV{IDF_PATH})
	#else()
  	#	set(IDF_PATH ${HOMEKIT_PATH}/esp-idf)
	#endif(DEFINED ENV{IDF_PATH})
	

	set(EXTRA_COMPONENT_DIRS ${HOMEKIT_PATH} ${HOMEKIT_PATH}/components ${HOMEKIT_PATH}/components/homekit "drivers libs src components_override")
	
#else()
#	set(EXTRA_COMPONENT_DIRS "drivers libs src")
#endif()


#include(${IDF_PATH}/tools/cmake/project.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)