/*
*    BluetoothServer.cpp
*    Class for handling Bluetooth connections
*
*/

using namespace std;

#include <stdio.h>
#include <string.h>

#include "Globals.h"
#include "BluetoothServer.h"

#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"

#include <esp_log.h>

static char tag[] = "BLEServer";

BluetoothServer_t::BluetoothServer_t() {
	BluetoothServerTaskHandle  = NULL;
	BluetoothClientTaskHandle  = NULL;
}

void BluetoothServer_t::Start() {
	ESP_LOGD(tag, "Start");

	BluetoothServerTaskHandle  = FreeRTOS::StartTask(BluetoothServerTask, "BluetoothServerTask", NULL, 8192);
	//BluetoothClientTaskHandle  = FreeRTOS::StartTask(BluetoothClientTask, "BluetoothClientTask", NULL, 8192);
}

void BluetoothServer_t::BluetoothServerTask(void *data)
{
	BLEDevice::init("LOOK.in " + Device.TypeToString() + " " + Device.IDToString());

	BLEServer*  pServer  = BLEDevice::createServer();

	// 180A - Device Information
	BLEService* pService = pServer->createService((uint16_t)0x180A);

	// Manufactorer Name String - 2A29
	BLECharacteristic* pCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)0x2A29), BLECharacteristic::PROPERTY_READ);
	pCharacteristic->setValue("LOOK.in");

	// Model number string - 0x2A24
	BLECharacteristic* pCharacteristicLN = pService->createCharacteristic(BLEUUID((uint16_t)0x2A24), BLECharacteristic::PROPERTY_READ);
	pCharacteristicLN->setValue("Plug");

	BLE2902* p2902Descriptor = new BLE2902();
	p2902Descriptor->setNotifications(true);
	pCharacteristic->addDescriptor(p2902Descriptor);

	pService->start();

	BLEAdvertising* pAdvertising = pServer->getAdvertising();
	pAdvertising->addServiceUUID(BLEUUID(pService->getUUID()));
	pAdvertising->start();

	ESP_LOGD(tag, "Advertising started!");

	FreeRTOS::Sleep(500000);
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	/**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGD(tag, "Advertised Device: %s", advertisedDevice.toString().c_str());
		if (advertisedDevice.getName() == "CC2650 SensorTag") {
			ESP_LOGD(tag, "Found our SensorTag device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			advertisedDevice.getScan()->stop();
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


void BluetoothServer_t::BluetoothClientTask(void *data)
{
	BLEDevice::init("ESP32");

	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(15);

	FreeRTOS::Sleep(500000);
}

void BluetoothServer_t::Stop() {
  ESP_LOGE(tag, "Stop");
}
