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

#include "BLE.h"
#include "GATT.h"

#include <esp_log.h>

using namespace std;

class BLEServer_t {
	public:
		BLEServer_t();

		void 				StartAdvertising(string Payload = "", bool ShoulUsePrivateMode = false);
		void 				StopAdvertising();

		void				SwitchToPublicMode();
		void				SwitchToPrivateMode();

		bool				IsInPrivateMode();

		static int 			GATTDeviceManufactorerCallback	(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceModelCallback			(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceIDCallback			(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceFirmwareCallback		(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceHardwareModelCallback	(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceControlFlagCallback	(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceWiFiCallback			(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
		static int 			GATTDeviceMQTTCallback			(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

	private:
		void				GATTSetup();

		bool				IsPrivateMode 	= false;
		bool				IsInited		= false;
};

#endif /* Bluetooth enabled */

#endif
