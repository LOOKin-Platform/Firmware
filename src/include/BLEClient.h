/*
*    BLEClient.h
*    Class for handling Bluetooth connections in client mode
*
*/

#ifndef BLECLIENT_H
#define BLECLIENT_H

#include <string>

#include <stdio.h>
#include <string.h>

#include "Converter.h"
#include "FreeRTOSTask.h"

#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "BLEServerGeneric.h"
#include "BLEUtils.h"
#include "BLE2902.h"

#include <esp_log.h>

using namespace std;

class BLEClient_t {
  public:
	BLEClient_t();

    void Scan();

  private:
	TaskHandle_t BluetoothClientTaskHandle;
	static void BluetoothClientTask(void *);
};

#endif /* BLUETOOTHBCLIENT_H */
