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
	"Data.cpp"
	"WebServer.cpp"
	"Wireless.cpp"
	"HomeKit/App.cpp"
	"HomeKit/DB.cpp"
	"HomeKit/HomeKit.cpp")

idf_component_register(SRCS "${srcs}"
                      INCLUDE_DIRS "include" "HomeKit/include"
                      REQUIRES "bt BLE Converter DateTime HTTPClient libJSON esp_http_server WiFi OTA HardwareIO ISR RMT IRLib mqtt UART Sleep mdns PowerManagement port")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_INFO")