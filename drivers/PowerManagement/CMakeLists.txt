
idf_component_register(SRCS "PowerManagement.cpp"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    PRIV_REQUIRES "esp_coex"
                    REQUIRES "esp_coex esp_wifi NimBLE main")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_INFO")