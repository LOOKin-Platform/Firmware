set(srcs 
    "esp8266_wrapper.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")