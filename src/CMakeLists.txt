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
	"HomeKitADK/App.cpp"
	"HomeKitADK/DB.cpp"
	"HomeKitADK/HomeKitADK.cpp"
	"HomeKitSDK/HomeKit.cpp"
	"HomeKitSDK/Custom.cpp")

idf_component_register(SRCS "${srcs}"
                      INCLUDE_DIRS "include" "HomeKitADK/include" "HomeKitSDK"
                      REQUIRES "bt BLE Converter DateTime HTTPClient libJSON esp_http_server WiFi OTA HardwareIO ISR RMT IRLib mqtt UART Sleep mdns PowerManagement platform hap_apple_profiles esp32-ds18b20 esp32-owb")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")