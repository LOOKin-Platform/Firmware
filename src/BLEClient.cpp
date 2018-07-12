/*
*    BLEClient.cpp
*    Class for handling Bluetooth connections in client mode
*
*/

#include "BLEClient.h"
#include "Globals.h"
#if defined(CONFIG_BT_ENABLED)

static char tag[] = "BLEClient";

// The remote service we wish to connect to.
static BLEUUID	serviceUUID((uint16_t)0x180A);
// The characteristic of the remote service we are interested in.
static BLEUUID	charUUID((uint16_t)0x2A29);

BLEClient_t::BLEClient_t() {
	BluetoothClientTaskHandle = NULL;
}

class MyClientCallback: public BLEClientCallbacks {
	void onConnect(BLEClientGeneric *pClient) {

		ESP_LOGD(tag, "<< onConnect");
		//map<string, BLERemoteService*> *Services = pClient->getServices(serviceUUID.getNative());

		//for (auto &item: *Services) {
		//	ESP_LOGI(tag, "%s", item.first.c_str());
		//}
		ESP_LOGD(tag, ">> onConnect");
		return;
	}

	void onDisconnect(BLEClientGeneric *pClient)  {
		return;
	}
};

class MyClient: public Task {
	void run(void* data) {
		ESP_LOGI(tag, "Service UDID: %s", serviceUUID.toString().c_str());

		BLEAddress* pAddress = (BLEAddress*)data;
		BLEClientGeneric*  pClient  = BLEDevice::CreateClient();

		MyClientCallback *ttt = new MyClientCallback();
		pClient->setClientCallbacks(ttt);
		// Connect to the remove BLE Server.
		pClient->connect(*pAddress);

		map<string, BLERemoteService*> *Services = pClient->getServices(serviceUUID.getNative());

		for (auto &item: *Services) {
			ESP_LOGI(tag, "%s", item.first.c_str());
		}

		// Obtain a reference to the service we are after in the remote BLE server.
		BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGE(tag, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
			return;
		}

		// Obtain a reference to the characteristic in the service of the remote BLE server.
		BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
		if (pRemoteCharacteristic == nullptr) {
			ESP_LOGE(tag, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
			return;
		}

		// Read the value of the characteristic.
		std::string value = pRemoteCharacteristic->readValue();
		ESP_LOGI(tag, "The characteristic value was: %s", value.c_str());

		/*
		while(1) {
			// Set a new value of the characteristic
			ESP_LOGI(tag, "Setting the new value");
			std::ostringstream stringStream;
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stringStream << "Time since boot: " << tv.tv_sec;
			pRemoteCharacteristic->writeValue(stringStream.str());

			FreeRTOS::Sleep(1000);
		}*/

		pClient->disconnect();

		ESP_LOGD(tag, "%s", pClient->toString().c_str());
		ESP_LOGD(tag, "-- End of task");
	} // run
}; // MyClient

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGD(tag, "Advertised Info : %s"		, advertisedDevice.toString().c_str());
		ESP_LOGD(tag, "Advertised Name : %s"		, advertisedDevice.getName().c_str());
		ESP_LOGD(tag, "Advertised Address : %s"		, advertisedDevice.getAddress().toString().c_str());
		ESP_LOGD(tag, "Advertised ManufacturerData: %s"	, advertisedDevice.getManufacturerData().c_str());
		ESP_LOGD(tag, "-------------------------------");

//		if (!advertisedDevice.haveManufacturerData()) return;

//		char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
//		if (string(pHex).substr(0,8) == "4c001002") {

		if ((advertisedDevice.getName().size() >= Settings.Bluetooth.DeviceNamePrefix.size()) &&
			(advertisedDevice.getName().substr(0, Settings.Bluetooth.DeviceNamePrefix.size()) == Settings.Bluetooth.DeviceNamePrefix)) {
			ESP_LOGD(tag, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());

			advertisedDevice.getScan()->stop();

			MyClient* pMyClient = new MyClient();
			pMyClient->setStackSize(20000);
			pMyClient->setPriority(255);
			pMyClient->setCore(1);
			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
			return;
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks

void BLEClient_t::Scan() {
	BLEDevice::Init("LOOK.in_" + DeviceType_t::ToString(Settings.eFuse.Type) + "_" + Device.IDToString());

	BLEScan *pBLEScan = BLEDevice::GetScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(30);

	while(1) {
		FreeRTOS::Sleep(1000);
	};
}

#endif /* Bluetooth enabled */
