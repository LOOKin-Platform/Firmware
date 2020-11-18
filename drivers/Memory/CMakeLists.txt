
idf_component_register(SRCS "NVSWrapper.cpp" "SPIFlash.cpp" "PartitionAPI.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "include"
                    REQUIRES "nvs_flash bt Converter")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_INFO")