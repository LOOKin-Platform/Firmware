
idf_component_register(SRCS "ds18b20-wrapper.cpp"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "esp32-ds18b20 esp32-owb freertos driver")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_ERROR")