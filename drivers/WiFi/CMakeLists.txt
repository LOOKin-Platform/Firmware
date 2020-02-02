set(srcs 
    "WiFi.cpp"
	"WiFiEventHandler.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES "FreeRTOSWrapper esp_wifi nvs_flash Converter PowerManagement")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")