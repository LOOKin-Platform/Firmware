set(SRC_FILES 
    "FreeRTOSTask.cpp"
	"FreeRTOSWrapper.cpp")

idf_component_register(SRCS "${SRC_FILES}"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "freertos")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_ERROR")