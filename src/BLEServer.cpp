/*
*    BLEServer.cpp
*    Class for handling Bluetooth connections
*
*/

#include "BLEServer.h"
#include "Globals.h"
#include "Converter.h"
#include "FreeRTOSTask.h"
#if defined(CONFIG_BT_ENABLED)

static char tag[] = "BLEServer";

BLEServer_t::BLEServer_t() {}

void ControlFlagCallback::onWrite(BLECharacteristic *pCharacteristic) {
	string Value = pCharacteristic->getValue();
	if (Value.length() > 0) {
		if (Value == "1") {
			ESP_LOGI(tag, "Received signal to switch on WiFi for interval");
			Network.WiFiConnect();
		}
	}
};

void WiFiNetworksCallback::onWrite(BLECharacteristic *pCharacteristic) {
	string Value = pCharacteristic->getValue();
	if (Value.length() > 0) {
		size_t Delimeter = Value.find(" ");

		if (Delimeter != string::npos && Delimeter < Value.size()) {
			ESP_LOGI(tag, "WiFi Data received. SSID: %s, Password: %s", Value.substr(0, Delimeter).c_str(), Value.substr(Delimeter+1).c_str());
			Network.AddWiFiNetwork(Value.substr(0, Delimeter), Value.substr(Delimeter+1));
		}
	}
}

void ServerCallback::onWrite(BLECharacteristic *pCharacteristic) {
	string Value = pCharacteristic->getValue();
	if (Value.length() > 0) {

		uint8_t 	EventCode 	= 0;
		uint32_t 	Operand		= 0;

		if (Value.length() >= 2) {
			EventCode 	= Converter::UintFromHexString<uint8_t>(Value.substr(0,2));

			Value = Value.substr(2);
			Converter::NormalizeHexString(Value, 8);
			Operand	= Converter::UintFromHexString<uint32_t>(Value);
		}

		ESP_LOGD(tag, "*********");
		ESP_LOGD(tag, "New value. EventCode: %X, Operand: %X", EventCode, Operand);
		ESP_LOGD(tag, "*********");
	}
};

void BLEServer_t::SetScanPayload(string Payload) {
	BLEAdvertisementData Data;
	Data.SetManufacturerData(Payload);

	pAdvertising->SetScanResponseData(Data);
}


void BLEServer_t::StartAdvertising(string Payload) {
	BLEDevice::Init(Settings.Bluetooth.DeviceNamePrefix + DeviceType_t::ToString(Settings.eFuse.Type) + " " + Device.IDToString());

	pServer  = BLEDevice::CreateServer();

	pServiceDevice = pServer->CreateService((uint16_t)0x180A);

	pCharacteristicManufacturer = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A29), BLECharacteristic::PROPERTY_READ);
	pCharacteristicManufacturer->setValue("LOOK.in");

	pCharacteristicModel = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A24), BLECharacteristic::PROPERTY_READ);
	pCharacteristicModel->setValue(Converter::ToHexString(Settings.eFuse.Type,2));

	pCharacteristicID = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A25), BLECharacteristic::PROPERTY_READ);
	pCharacteristicID->setValue(Converter::ToHexString(Settings.eFuse.DeviceID,8));

	pCharacteristicFirmware = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A26), BLECharacteristic::PROPERTY_READ);
	pCharacteristicFirmware->setValue(Settings.FirmwareVersion);

	pCharacteristicControlFlag = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x3000), BLECharacteristic::PROPERTY_WRITE);
	pCharacteristicControlFlag->setCallbacks(new ControlFlagCallback());

	pCharacteristicWiFiNetworks = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x4000), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
	pCharacteristicWiFiNetworks->setValue(Converter::VectorToString(Network.GetSavedWiFiList(), ","));
	pCharacteristicWiFiNetworks->setCallbacks(new WiFiNetworksCallback());

	// FFFF - Sensors and Commands Information
	pServiceActuators = pServer->CreateService((uint16_t)0xFFFF);

	/*
	for (auto& Sensor : Sensors) {
		BLECharacteristic* pCharacteristicSensor = pServiceActuators->createCharacteristic(BLEUUID((uint16_t)Sensor->ID), BLECharacteristic::PROPERTY_READ);
		pCharacteristicSensor->setValue(Converter::ToHexString(Sensor->GetValue().Value,2));
	}

	for (auto& Command : Commands) {
		BLECharacteristic* pCharacteristicCommand = pServiceActuators->createCharacteristic(BLEUUID((uint16_t)Command->ID), BLECharacteristic::PROPERTY_WRITE);
		pCharacteristicCommand->setCallbacks(new ServerCallback());
	}
	*/

	pServiceDevice		->Start();
	pServiceActuators	->Start();

	pAdvertising = pServer->GetAdvertising();

	pAdvertising->AddServiceUUID(BLEUUID(pServiceDevice->GetUUID()));
	pAdvertising->AddServiceUUID(BLEUUID(pServiceActuators->GetUUID()));

	if (Payload != "")
		SetScanPayload(Payload);

	pAdvertising->Start();

	ESP_LOGI(tag, "<< StartAdvertising");
}

void BLEServer_t::StopAdvertising() {
	pAdvertising->Stop();
	//pServiceDevice->Stop();
	//pServiceActuators->Stop();

	BLEDevice::Deinit();

	delete(pCharacteristicManufacturer);
	delete(pCharacteristicModel);
	delete(pCharacteristicID);
	delete(pCharacteristicFirmware);
	delete(pCharacteristicControlFlag);
	delete(pCharacteristicWiFiNetworks);
	delete(pServiceDevice);
	delete(pServiceActuators);

	delete(pServer);

	//delete(pAdvertising);

	ESP_LOGI(tag, "Stop advertising");
}

#endif /* Bluetooth enabled */
