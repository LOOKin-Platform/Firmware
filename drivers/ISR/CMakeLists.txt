
idf_component_register(SRCS "ISR.cpp"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "driver")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_INFO")