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

class ControlFlagCallback	: public BLECharacteristicCallbacks 	{ void onWrite(BLECharacteristic *pCharacteristic); };
class WiFiNetworksCallback	: public BLECharacteristicCallbacks 	{ void onWrite(BLECharacteristic *pCharacteristic); };
class ServerCallback		: public BLECharacteristicCallbacks 	{ void onWrite(BLECharacteristic *pCharacteristic); };

class BLEServer_t {
	public:
		BLEServer_t();
		void Init();

		void SetScanPayload(string Payload = "");

		void StartAdvertising(string Payload);
		void StopAdvertising();

		bool IsRunning() { return BLEDevice::IsRunning(); }

	private:
		BLEServerGeneric	*pServer;
		BLEAdvertising		*pAdvertising;

		BLEService			*pServiceDevice;
		BLECharacteristic	*pCharacteristicManufacturer;
		BLECharacteristic	*pCharacteristicModel;
		BLECharacteristic	*pCharacteristicID;
		BLECharacteristic	*pCharacteristicFirmware;
		BLECharacteristic	*pCharacteristicControlFlag;
		BLECharacteristic	*pCharacteristicWiFiNetworks;

		BLEService			*pServiceActuators;

		map<uint8_t, BLECharacteristic*> SensorsCharacteristics;
		map<uint8_t, BLECharacteristic*> CommandsCharacteristics;
};

#endif /* Bluetooth enabled */

#endif
