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

#include <esp_log.h>

using namespace std;

class BLEClient_t {
	public:
		BLEClient_t();

		//void Scan(uint32_t Duration);
		//void ScanStop();

		//vector<BLEAddress>	ScanDevicesProcessed = {};
		//uint32_t			ScanStartTime;
	private:
		//BLEScan				*pBLEScan;
};

#endif /* BLUETOOTHBCLIENT_H */
