/*
*    BLEServer.cpp
*    Class for handling Bluetooth connections
*
*/

#include "Globals.h"
#include "BLEServer.h"
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
		JSON JSONItem(Value);

		map<string,string> Params;

		ESP_LOGE(tag, "Value %s", Value.c_str());


		if (JSONItem.GetType() == JSON::RootType::Object)
			Params = JSONItem.GetItems();

		if (!(Params.count("s") && Params.count("p")))
		{
			vector<string> Parts = Converter::StringToVector(Value," ");
			Params["s"] = Parts[0].c_str();
			Params["p"] = Parts[1].c_str();
		}

		if (!(Params.count("s") && Params.count("p")))
		{
			ESP_LOGE(tag, "WiFi characteristics error input format");
			return;
		}

		ESP_LOGD(tag, "WiFi Data received. SSID: %s, Password: %s", Params["s"].c_str(), Params["p"].c_str());
		Network.AddWiFiNetwork(Params["s"], Params["p"]);

		//If device in AP mode or can't connect as Station - try to connect with new data
		if ((WiFi.GetMode() == WIFI_MODE_STA_STR && WiFi.GetConnectionStatus() == UINT8_MAX) || (WiFi.GetMode() == WIFI_MODE_AP_STR))
			Network.WiFiConnect(Params["s"], true);
	}
}

void MQTTCredentialsCallback::onWrite(BLECharacteristic *pCharacteristic) {
	string Value = pCharacteristic->getValue();

	if (Value.length() > 0) {
		vector<string> Parts = Converter::StringToVector(Value," ");

		if (Parts.size() != 2) {
			ESP_LOGE(tag, "MQTT credentials error input format");
			return;
		}

		ESP_LOGD(tag, "MQTT credentials data received. ClientID: %s, ClientSecret: %s", Parts[0].c_str(), Parts[1].c_str());
		MQTT.ChangeOrSetCredentialsBLE(Parts[0], Parts[1]);
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


void BLEServer_t::StartAdvertising(string Payload, bool ShouldUsePrivateMode) {
	ESP_LOGI(tag, ">> StartAdvertising");

	string BLEDeviceName = DeviceType_t::ToString(Settings.eFuse.Type);

	if (BLEDeviceName.size() < 4)
		BLEDeviceName += "!";

	BLEDeviceName = Settings.Bluetooth.DeviceNamePrefix + BLEDeviceName + "_" + Device.IDToString();

	BLEDevice::Init(BLEDeviceName);

	IsPrivateMode = ShouldUsePrivateMode;

	if (ShouldUsePrivateMode)
		BLEDevice::SetPower(Settings.Bluetooth.PrivateModePower, ESP_BLE_PWR_TYPE_ADV);
	else
		BLEDevice::SetPower(Settings.Bluetooth.PublicModePower, ESP_BLE_PWR_TYPE_ADV);

	pServer  = BLEDevice::CreateServer();

	pServiceDevice = pServer->CreateService((uint16_t)0x180A);

	pCharacteristicManufacturer = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A29), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST);
	pCharacteristicManufacturer->setValue("LOOK.in");

	pCharacteristicModel = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A24), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST);
	pCharacteristicModel->setValue(Converter::ToHexString(Settings.eFuse.Type,2));

	pCharacteristicID = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A25), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST);
	pCharacteristicID->setValue(Converter::ToHexString(Settings.eFuse.DeviceID,8));

	pCharacteristicFirmware = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x2A26), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST);
	pCharacteristicFirmware->setValue(Settings.FirmwareVersion);

	pCharacteristicControlFlag = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x3000), BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_BROADCAST);
	pCharacteristicControlFlag->setCallbacks(new ControlFlagCallback());

	if (IsPrivateMode) {
		pCharacteristicWiFiNetworks = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x4000), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_BROADCAST);
		pCharacteristicWiFiNetworks->setValue(Converter::VectorToString(Network.GetSavedWiFiList(), ","));
		pCharacteristicWiFiNetworks->setCallbacks(new WiFiNetworksCallback());

		pCharacteristicMQTTCredentials = pServiceDevice->CreateCharacteristic(BLEUUID((uint16_t)0x5000), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_BROADCAST);
		pCharacteristicMQTTCredentials->setValue(MQTT.GetClientID());
		pCharacteristicMQTTCredentials->setCallbacks(new MQTTCredentialsCallback());
	}

	// FFFF - Sensors and Commands Information
	//pServiceActuators = pServer->CreateService((uint16_t)0xFFFF);

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
	//pServiceActuators	->Start();

	pAdvertising = pServer->GetAdvertising();

	pAdvertising->AddServiceUUID(BLEUUID(pServiceDevice->GetUUID()));
	//pAdvertising->AddServiceUUID(BLEUUID(pServiceActuators->GetUUID()));

	if (Payload != "")
		SetScanPayload(Payload);

	pAdvertising->Start();

	ESP_LOGI(tag, "<< StartAdvertising");
}

void BLEServer_t::StopAdvertising() {
	if (!IsRunning())
		return;

	pAdvertising->Stop();
	//pServiceDevice->Stop();
	//pServiceActuators->Stop();

	BLEDevice::Deinit();

	delete(pCharacteristicManufacturer);
	delete(pCharacteristicModel);
	delete(pCharacteristicID);
	delete(pCharacteristicFirmware);
	delete(pCharacteristicControlFlag);

	if (IsPrivateMode) {
		delete(pCharacteristicWiFiNetworks);
		delete(pCharacteristicMQTTCredentials);
	}

	delete(pServiceDevice);
	//delete(pServiceActuators);

	delete(pServer);

	//delete(pAdvertising);

	ESP_LOGI(tag, "Stop advertising");
}

void BLEServer_t::SwitchToPublicMode() {
	if (IsRunning() && IsPrivateMode) {
		StopAdvertising();
		FreeRTOS::Sleep(1000);
		StartAdvertising();
	}
}

void BLEServer_t::SwitchToPrivateMode() {
	if (IsRunning() && !IsPrivateMode) {
		StopAdvertising();
		FreeRTOS::Sleep(1000);
		StartAdvertising("", true);
	}
}

bool BLEServer_t::IsInPrivateMode() {
	return IsPrivateMode;
}


#endif /* Bluetooth enabled */
