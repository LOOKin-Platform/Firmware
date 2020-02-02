set(srcs 
    "GeneralUtils.cpp"
	"BLE2902.cpp"
	"BLE2904.cpp"
	"BLEAddress.cpp"
	"BLEAdvertisedDevice.cpp"
	"BLEAdvertising.cpp"
	"BLECharacteristic.cpp"
	"BLECharacteristicMap.cpp"
	"BLEClientGeneric.cpp"
	"BLEDescriptor.cpp"
	"BLEDescriptorMap.cpp"
	"BLEDevice.cpp"
	"BLEExceptions.cpp"
	"BLEHIDDevice.cpp"
	"BLERemoteCharacteristic.cpp"
	"BLERemoteDescriptor.cpp"
	"BLERemoteService.cpp"
	"BLEScan.cpp"
	"BLESecurity.cpp"
	"BLEServerGeneric.cpp"
	"BLEService.cpp"
	"BLEServiceMap.cpp"
	"BLEUtils.cpp"
	"BLEUUID.cpp"
	"BLEValue.cpp")
	
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "include"
                    REQUIRES "bt FreeRTOSWrapper")
                    
target_compile_options(${COMPONENT_LIB} PRIVATE "-D LOG_LOCAL_LEVEL=ESP_LOG_ERROR")