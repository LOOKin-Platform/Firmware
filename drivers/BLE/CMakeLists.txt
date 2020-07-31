
idf_component_register(SRCS "BLE.cpp" "GATT.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "include"
                    REQUIRES "nvs_flash bt Converter FreeRTOSWrapper")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_INFO")