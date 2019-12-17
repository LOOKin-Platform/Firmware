/*
*    BLEServer.h
*    Class for handling Bluetooth connections
*
*/
#ifndef BLESERVER_H
#define BLESERVER_H

#if defined(CONFIG_BT_ENABLED)

#include <string>
#include <stdio.h>

#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "BLEServerGeneric.h"
#include "BLEUtils.h"
#include "BLE2902.h"

#include <esp_log.h>

using namespace std;

class ControlFlagCallback		: public BLECharacteristicCallbacks 	{ void OnWrite(BLECharacteristic *pCharacteristic); };
class WiFiNetworksCallback		: public BLECharacteristicCallbacks 	{ void OnWrite(BLECharacteristic *pCharacteristic); };
class MQTTCredentialsCallback	: public BLECharacteristicCallbacks 	{ void OnWrite(BLECharacteristic *pCharacteristic); };
class ServerCallback			: public BLECharacteristicCallbacks 	{ void OnWrite(BLECharacteristic *pCharacteristic); };

class BLEServer_t {
	public:
		BLEServer_t();

		void 				SetScanPayload(string Payload = "");

		void 				StartAdvertising(string Payload = "", bool ShoulUsePrivateMode = false);
		void 				StopAdvertising();

		bool 				IsRunning() { return BLEDevice::IsRunning(); }

		void				SwitchToPublicMode();
		void				SwitchToPrivateMode();

		bool				IsInPrivateMode();

	private:
		bool				IsPrivateMode = false;


		BLEServerGeneric	*pServer;
		BLEAdvertising		*pAdvertising;

		BLEService			*pServiceDevice;
		BLECharacteristic	*pCharacteristicManufacturer;
		BLECharacteristic	*pCharacteristicModel;
		BLECharacteristic	*pCharacteristicID;
		BLECharacteristic	*pCharacteristicFirmware;
		BLECharacteristic	*pCharacteristicControlFlag;
		BLECharacteristic	*pCharacteristicWiFiNetworks;
		BLECharacteristic	*pCharacteristicMQTTCredentials;

		BLEService			*pServiceActuators;

		map<uint8_t, BLECharacteristic*> SensorsCharacteristics;
		map<uint8_t, BLECharacteristic*> CommandsCharacteristics;
};

#endif /* Bluetooth enabled */

#endif
