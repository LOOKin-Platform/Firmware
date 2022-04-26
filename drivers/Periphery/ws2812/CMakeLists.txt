set(srcs 
    "ws2812.cpp"
	"led_strip_rmt_ws2812.c")
	
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "HardwareIO")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")