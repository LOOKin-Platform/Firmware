set(srcs 
    "API.cpp"
	"Automation.cpp"
	"BLEClient.cpp"
	"BLEServer.cpp"
	"Commands.cpp"
	"Device.cpp"
	"Log.cpp"
	"main.cpp"
	"RemoteControl.cpp"
	"LocalMQTT.cpp"
	"Network.cpp"
	"Query.cpp"
	"Scenarios.cpp"
	"Sensors.cpp"
	"Settings.cpp"
	"Storage.cpp"
	"Data.cpp"
	"WebServer.cpp"
	"Wireless.cpp"
	"BootAndRestore.cpp"
	"HomeKit/HomeKit.cpp"
	"HomeKit/Custom.cpp"
	"HomeKit/HomeKitAPI.cpp")

idf_component_register(SRCS "${srcs}"
                      INCLUDE_DIRS "HomeKit/include" "include" 
                      REQUIRES "bt Converter DateTime HTTPClient libJSON esp_http_server WiFi OTA HardwareIO ISR RMT IRLib mqtt UART Sleep mdns PowerManagement esp_hap_platform esp_hap_apple_profiles esp32-ds18b20 esp32-owb esp-bme280 hdc1080 NimBLE ds18b20-wrapper ws2812")

target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_DEBUG")