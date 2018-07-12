/*
 *    BLEDevice.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEDEVICE_H_
#define DRIVERS_BLEDEVICE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_gap_ble_api.h> // ESP32 BLE
#include <esp_gattc_api.h>   // ESP32 BLE
#include <map>               // Part of C++ STL
#include <string>
#include <esp_bt.h>

#include "BLEServerGeneric.h"
#include "BLEClientGeneric.h"
#include "BLEUtils.h"
#include "BLEScan.h"
#include "BLEAddress.h"

using namespace std;

/**
 * @brief %BLE functions.
 */
class BLEDevice {
	public:
		static void					Init(string deviceName);	// Initialize the local BLE environment.
		static void					Deinit();

		static BLEClientGeneric*	CreateClient();    // Create a new BLE client.
		static BLEServerGeneric*	CreateServer();    // Create a new BLE server.
		static BLEAddress  			GetAddress();      // Retrieve our own local BD address.
		static BLEScan*				GetScan();         // Get the scan object
		static string				GetValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID); // Get the value of a characteristic of a service on a server.

		static void					SetPower(esp_power_level_t powerLevel);  // Set our power level.
		static void					SetValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID, string value);   // Set the value of a characteristic on a service on a server.
		static string 				toString();        // Return a string representation of our device.
		static void					WhiteListAdd(BLEAddress address);    // Add an entry to the BLE white list.
		static void					WhiteListRemove(BLEAddress address); // Remove an entry from the BLE white list.

	    static bool					IsRunning() { return IsInitialized; }

	private:
		static BLEServerGeneric		*m_pServer;
		static BLEScan				*m_pScan;
		static BLEClientGeneric		*m_pClient;

		static bool					IsInitialized;

		static esp_gatt_if_t		getGattcIF();

		static void gattClientEventHandler	(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);
		static void gattServerEventHandler	(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
		static void gapEventHandler			(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
}; // class BLE

#endif // CONFIG_BT_ENABLED
#endif /* DRIVERS_BLEDEVICE_H_ */
