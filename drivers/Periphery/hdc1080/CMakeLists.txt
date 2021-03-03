set(srcs 
    "hdc1080.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "driver soc hdc1080_esp8266_wrapper")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")