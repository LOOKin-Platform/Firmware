#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src/drivers
COMPONENT_ADD_INCLUDEDIRS += $(PROJECT_PATH)/src/drivers

COMPONENT_ADD_LDFLAGS=-lstdc++ -l$(COMPONENT_NAME)
