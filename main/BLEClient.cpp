/*
*    BLEClient.cpp
*    Class for handling Bluetooth connections in client mode
*
*/

#include "BLEClient.h"
#include "Globals.h"

//static char tag[] = "BLEClient";

BLEClient_t::BLEClient_t() {
}

/*
class MyClientCallback: public BLEClientCallbacks {
	uint32_t RemainingTime;

	void OnConnect(BLEClientGeneric *pClient) {
		BLEClient.ScanDevicesProcessed.push_back(pClient->GetPeerAddress());
		RemainingTime = BLEDevice::GetScan()->ScanDuration - ((Time::Uptime() - BLEClient.ScanStartTime));
		return;
	}

	void OnDisconnect(BLEClientGeneric *pClient)  {
		if (RemainingTime > 0)
			BLEClient.Scan(RemainingTime);

		return;
	}
};

class SecretCodeClient: public Task {
	void Run(void* data) {
		BLEAddress* 		pAddress = (BLEAddress*)data;
		BLEClientGeneric* 	pClient  = BLEDevice::CreateClient();

		pClient->SetClientCallbacks(new MyClientCallback());
		pClient->Connect(*pAddress);

		BLERemoteService *SecretCodeService = pClient->GetService(BLEUUID(Settings.Bluetooth.SecretCodeServiceUUID));

		if (SecretCodeService == nullptr) {
			ESP_LOGE(tag, "Failed to find secret code service UUID: %s",Settings.Bluetooth.SecretCodeServiceUUID.c_str());
		}
		else {
			BLERemoteCharacteristic* SecretCodeCharacteristic = SecretCodeService->GetCharacteristic(BLEUUID(Settings.Bluetooth.SecretCodeUUID));

			if (SecretCodeCharacteristic == nullptr)
				ESP_LOGE(tag, "Failed to find secret code characteristic");
			else
				if (SecretCodeCharacteristic->CanWrite()) {
					//SecretCodeCharacteristic->WriteValue(BLEServer.SecretCodeString(), true);
				}
		}

		delete(pAddress);

		//ESP_LOGD(tag, "Wroted secret code to remote BLE Server %s", BLEServer.SecretCodeString().c_str());

		pClient->Disconnect();
		pClient->ClearServices();
	} // run
}; // MyClient

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {

		//invalid cast from Const to non-const

	    //if (find(BLEClient.ScanDevicesProcessed.begin(),
	    //		BLEClient.ScanDevicesProcessed.end(), advertisedDevice.GetAddress()) != BLEClient.ScanDevicesProcessed.end())
	    //	return;

		bool CorrectDevice = false;

		ESP_LOGI("tag","%s", advertisedDevice.ToString().c_str());

		if (advertisedDevice.GetName().find(Settings.Bluetooth.DeviceNamePrefix) == 0)
			CorrectDevice = true;

		if (advertisedDevice.HaveServiceUUID())
			if (advertisedDevice.GetServiceUUID().ToString() == BLEUUID(Settings.Bluetooth.SecretCodeServiceUUID).ToString())
				CorrectDevice = true;

		if (CorrectDevice) {
				ESP_LOGI(tag, "Device Founded");

				advertisedDevice.GetScan()->Stop();

				SecretCodeClient *pSecretCodeClient = new SecretCodeClient();
				pSecretCodeClient->SetStackSize(10000);
				pSecretCodeClient->SetPriority(255);
				pSecretCodeClient->SetCore(0);

				pSecretCodeClient->Start(new BLEAddress(*advertisedDevice.GetAddress().GetNative()));

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
	pBLEScan->SetActiveScan(false);
	pBLEScan->SetWindow(1000);
	pBLEScan->SetInterval(5);
	pBLEScan->Start(Duration);
}

void BLEClient_t::ScanStop() {
	pBLEScan->Stop();
}
*/
