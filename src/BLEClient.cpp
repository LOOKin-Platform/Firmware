/*
*    BLEClient.cpp
*    Class for handling Bluetooth connections in client mode
*
*/

#include "BLEClient.h"
#include "Globals.h"
#if defined(CONFIG_BT_ENABLED)

static char tag[] = "BLEClient";

BLEClient_t::BLEClient_t() {
	BluetoothClientTaskHandle = NULL;
}

class MyClientCallback: public BLEClientCallbacks {
	uint32_t RemainingTime;

	void onConnect(BLEClientGeneric *pClient) {
		BLEClient.ScanDevicesProcessed.push_back(pClient->getPeerAddress());
		RemainingTime = BLEDevice::GetScan()->ScanDuration - ((Time::Uptime() - BLEClient.ScanStartTime));
		return;
	}

	void onDisconnect(BLEClientGeneric *pClient)  {
		if (RemainingTime > 0)
			BLEClient.Scan(RemainingTime);

		return;
	}
};

class SecretCodeClient: public Task {
	void Run(void* data) {
		ESP_LOGI(tag,"!!!!!");

		BLEAddress* 		pAddress = (BLEAddress*)data;
		BLEClientGeneric* 	pClient  = BLEDevice::CreateClient();

		pClient->setClientCallbacks(new MyClientCallback());
		pClient->Connect(*pAddress);

		BLERemoteService *SecretCodeService = pClient->getService(BLEUUID(Settings.Bluetooth.SecretCodeServiceUUID));

		if (SecretCodeService == nullptr) {
			ESP_LOGE(tag, "Failed to find secret code service UUID: %s",Settings.Bluetooth.SecretCodeServiceUUID.c_str());
		}
		else {
			BLERemoteCharacteristic* SecretCodeCharacteristic = SecretCodeService->GetCharacteristic(BLEUUID(Settings.Bluetooth.SecretCodeUUID));

			if (SecretCodeCharacteristic == nullptr)
				ESP_LOGE(tag, "Failed to find secret code characteristic");
			else
				if (SecretCodeCharacteristic->canWrite())
					SecretCodeCharacteristic->WriteValue(BLEServer.SecretCodeString(), true);
		}

		delete(pAddress);

		ESP_LOGD(tag, "Wroted secret code to remote BLE Server %s", BLEServer.SecretCodeString().c_str());

		pClient->Disconnect();
		pClient->ClearServices();
	} // run
}; // MyClient

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
	    if (find(BLEClient.ScanDevicesProcessed.begin(),
	    		BLEClient.ScanDevicesProcessed.end(), advertisedDevice.getAddress()) != BLEClient.ScanDevicesProcessed.end())
	    	return;

		bool CorrectDevice = false;

		if (advertisedDevice.getName().find(Settings.Bluetooth.DeviceNamePrefix) == 0)
			CorrectDevice = true;

		if (advertisedDevice.haveServiceUUID())
			if (advertisedDevice.getServiceUUID().toString() == BLEUUID(Settings.Bluetooth.SecretCodeServiceUUID).toString())
				CorrectDevice = true;

		ESP_LOGD("Founded", "Flag %d: Device: %s", CorrectDevice, advertisedDevice.toString().c_str());

		if (advertisedDevice.haveRSSI())
			if (CorrectDevice && advertisedDevice.getRSSI() > Settings.Bluetooth.SecretCodeRSSIMinimun) {
				advertisedDevice.getScan()->Stop();

				SecretCodeClient *pSecretCodeClient = new SecretCodeClient();
				pSecretCodeClient->SetStackSize(10000);
				pSecretCodeClient->SetPriority(255);
				pSecretCodeClient->SetCore(0);

				pSecretCodeClient->Start(new BLEAddress(*advertisedDevice.getAddress().GetNative()));

				return;
			}
	} // onResult

}; // MyAdvertisedDeviceCallbacks

void BLEClient_t::Scan(uint32_t Duration) {
	BLEDevice::Init("LOOK.in_" + DeviceType_t::ToString(Settings.eFuse.Type) + "_" + Device.IDToString());

	ScanStartTime = Time::Uptime();

	if (pBLEScan == nullptr)
		pBLEScan = BLEDevice::GetScan();

	ScanDevicesProcessed.clear();

	pBLEScan->SetAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
	pBLEScan->SetActiveScan(true);
	pBLEScan->SetWindow(1000);
	pBLEScan->SetInterval(1000);
	pBLEScan->Start(Duration);
}

void BLEClient_t::ScanStop() {
	pBLEScan->Stop();
}

#endif /* Bluetooth enabled */
