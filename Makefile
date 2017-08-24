#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := firmware
CFLAGS += -save-temps

#COMPONENT_SRCDIRS := src drivers
#COMPONENT_ADD_INCLUDEDIRS := . drivers
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src/drivers

include $(IDF_PATH)/make/project.mk
