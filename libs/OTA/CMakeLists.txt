idf_component_register(SRCS "OTA.cpp"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "esp_https_ota app_update HTTPClient")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")