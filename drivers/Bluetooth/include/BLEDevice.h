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
typedef void (*gap_event_handler)	(esp_gap_ble_cb_event_t  	event, esp_ble_gap_cb_param_t* param);
typedef void (*gattc_event_handler)	(esp_gattc_cb_event_t 		event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);
typedef void (*gatts_event_handler)	(esp_gatts_cb_event_t 		event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t* param);

class BLEDevice {
	public:
		static void					Init(string deviceName);	// Initialize the local BLE environment.
		static void					Deinit(bool release_memory = false);

		static void					SetSleep(bool);

		static BLEClientGeneric*	CreateClient();    // Create a new BLE client.
		static BLEServerGeneric*	CreateServer();    // Create a new BLE server.
		static BLEAddress  			GetAddress();      // Retrieve our own local BD address.
		static BLEScan*				GetScan();         // Get the scan object

		static string				GetValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID); // Get the value of a characteristic of a service on a server.
		static void					SetValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID, string value);   // Set the value of a characteristic on a service on a server.

		static void					SetPower(esp_power_level_t powerLevel, esp_ble_power_type_t PowerType = ESP_BLE_PWR_TYPE_DEFAULT);  // Set our power level.
		static string 				ToString();        // Return a string representation of our device.
		static void					WhiteListAdd(BLEAddress address);    // Add an entry to the BLE white list.
		static void					WhiteListRemove(BLEAddress address); // Remove an entry from the BLE white list.

	    static bool					IsRunning() { return IsInitialized; }
		static bool       			GetInitialized(); // Returns the state of the device, is it initialized or not?

		static void		   			SetEncryptionLevel(esp_ble_sec_act_t level);
		static void		   			SetSecurityCallbacks(BLESecurityCallbacks* pCallbacks);

		/* move advertising to BLEDevice for saving ram and flash in beacons */
		static BLEAdvertising*		GetAdvertising();
		static void					StartAdvertising();
		static uint16_t				m_appId;

		/* multi connect */
		static map<uint16_t, conn_status_t>
									GetPeerDevices(bool client);
		static void 				AddPeerDevice(void* peer, bool is_client, uint16_t conn_id);
		static void 				UpdatePeerDevice(void* peer, bool _client, uint16_t conn_id);
		static void 				RemovePeerDevice(uint16_t conn_id, bool client);
		static BLEClientGeneric* 	GetClientByGattIf(uint16_t conn_id);
		static void 				SetCustomGapHandler(gap_event_handler handler);
		static void 				SetCustomGattcHandler(gattc_event_handler handler);
		static void 				SetCustomGattsHandler(gatts_event_handler handler);


		static esp_err_t   			SetMTU(uint16_t mtu);
		static uint16_t	   			GetMTU();
		static uint16_t				m_localMTU;

		static esp_ble_sec_act_t 	m_securityLevel;


	private:
		static BLEServerGeneric		*m_pServer;
		static BLEScan				*m_pScan;
		static BLESecurityCallbacks	*m_securityCallbacks;
		static BLEAdvertising		*m_bleAdvertising;

		static std::map<uint16_t, conn_status_t> m_connectedClientsMap;

		static bool					IsInitialized;

		static esp_gatt_if_t		getGattcIF();

		static void gattClientEventHandler(
			esp_gattc_cb_event_t      event,
			esp_gatt_if_t             gattc_if,
			esp_ble_gattc_cb_param_t* param);

		static void gattServerEventHandler(
		   esp_gatts_cb_event_t      event,
		   esp_gatt_if_t             gatts_if,
		   esp_ble_gatts_cb_param_t* param);

		static void gapEventHandler(
			esp_gap_ble_cb_event_t  event,
			esp_ble_gap_cb_param_t* param);

	public:
		/* custom gap and gatt handlers for flexibility */
		static gap_event_handler m_customGapHandler;
		static gattc_event_handler m_customGattcHandler;
		static gatts_event_handler m_customGattsHandler;

}; // class BLE

#endif // CONFIG_BT_ENABLED
#endif /* DRIVERS_BLEDEVICE_H_ */
