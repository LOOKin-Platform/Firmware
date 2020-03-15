# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#set(SRC_DIRS . src drivers libs src/handlers src/ulp)
#set(EXTRA_COMPONENT_DIRS . src drivers libs src/handlers src/ulp)
set(EXTRA_COMPONENT_DIRS "drivers libs src")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firmware)