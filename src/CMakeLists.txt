set(srcs 
    "API.cpp"
	"Automation.cpp"
	"BLEClient.cpp"
	"BLEServer.cpp"
	"Commands.cpp"
	"Device.cpp"
	"Log.cpp"
	"main.cpp"
	"MQTT.cpp"
	"Network.cpp"
	"Query.cpp"
	"Scenarios.cpp"
	"Sensors.cpp"
	"Settings.cpp"
	"Storage.cpp"
	"WebServer.cpp"
	"Wireless.cpp")

idf_component_register(SRCS "${srcs}"
                      INCLUDE_DIRS "include"
                      REQUIRES "Bluetooth Converter DateTime HTTPClient libJSON esp_http_server WiFi OTA HardwareIO ISR RMT IRLib mqtt bt UART Sleep mdns PowerManagement")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_ERROR")