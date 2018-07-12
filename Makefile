#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := firmware
CFLAGS += -save-temps

COMPONENT_SRCDIRS := . src drivers libs src/handlers src/ulp
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/drivers
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/libs
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src/handlers
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src/ulp

include $(IDF_PATH)/make/project.mk
