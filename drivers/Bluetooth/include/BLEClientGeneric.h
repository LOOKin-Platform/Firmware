/*
 *    BLEClientGeneric.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLECLIENT_H_
#define DRIVERS_BLECLIENT_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <esp_gattc_api.h>
#include <string.h>
#include <map>
#include <string>
#include "BLEExceptions.h"
#include "BLERemoteService.h"
#include "BLEService.h"
#include "BLEAddress.h"
#include "BLESecurity.h"


class BLERemoteService;
class BLEClientCallbacks;
class BLEAdvertisedDevice;

using namespace std;

/**
 * @brief A model of a %BLE client.
 */
class BLEClientGeneric {
	public:
		BLEClientGeneric();
		~BLEClientGeneric();

		bool							Connect(BLEAdvertisedDevice* device);
		bool							Connect(BLEAddress address, esp_ble_addr_type_t type = BLE_ADDR_TYPE_PUBLIC);   // Connect to the remote BLE Server
		void 							Disconnect();                  				// Disconnect from the remote BLE Server

		BLEAddress 						GetPeerAddress();     						// Get the address of the remote BLE Server
		int 							GetRssi();                     				// Get the RSSI of the remote BLE Server
		map<string, BLERemoteService*>* GetServices(); 								// Get a map of the services offered by the remote BLE Server

		BLERemoteService* 				GetService(const char* uuid); 				// Get a reference to a specified service offered by the remote BLE server.
		BLERemoteService* 				GetService(BLEUUID uuid); 					// Get a reference to a specified service offered by the remote BLE server.
		string 							GetValue(BLEUUID serviceUUID, BLEUUID characteristicUUID); // Get the value of a given characteristic at a given service.

		void 							HandleGAPEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);

		bool 							IsConnected(); // Return true if we are connected.
		void 							SetClientCallbacks(BLEClientCallbacks *pClientCallbacks, bool deleteCallbacks = true);
		void 							SetValue(BLEUUID serviceUUID, BLEUUID characteristicUUID, string value); // Set the value of a given characteristic at a given service.

		void 							setEncryptionLevel(esp_ble_sec_act_t level);
		void 							setSecurityCallbacks(BLESecurityCallbacks* pCallbacks);

		string 							ToString();    // Return a string representation of this client.

		uint16_t						GetConnId();
		esp_gatt_if_t					GetGattcIf();
		uint16_t 						GetMTU();

		uint16_t m_appId;

		void ClearServices();   // Clear any existing services.
	private:
		friend class BLEDevice;
		friend class BLERemoteService;
		friend class BLERemoteCharacteristic;
		friend class BLERemoteDescriptor;

		void 							GattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);

		BLEAddress    					m_peerAddress = BLEAddress((uint8_t*)"\0\0\0\0\0\0");   // The BD address of the remote server.
		uint16_t      					m_conn_id;
		esp_gatt_if_t 					m_gattc_if;
		bool          					m_haveServices = false;    // Have we previously obtain the set of services from the remote server.
		bool          					m_isConnected = false;     // Are we currently connected.
		bool		  					m_deleteCallbacks = true;

		BLEClientCallbacks 				*m_pClientCallbacks = nullptr;
		FreeRTOS::Semaphore 			m_semaphoreRegEvt        = FreeRTOS::Semaphore("RegEvt");
		FreeRTOS::Semaphore 			m_semaphoreOpenEvt       = FreeRTOS::Semaphore("OpenEvt");
		FreeRTOS::Semaphore 			m_semaphoreSearchCmplEvt = FreeRTOS::Semaphore("SearchCmplEvt");
		FreeRTOS::Semaphore 			m_semaphoreRssiCmplEvt   = FreeRTOS::Semaphore("RssiCmplEvt");
		map<std::string, BLERemoteService*>
										m_servicesMap;
		map<BLERemoteService*, uint16_t>m_servicesMapByInstID;

		uint16_t m_mtu = 23;
};
// class BLEDevice

/**
 * @brief Callbacks associated with a %BLE client.
 */
class BLEClientCallbacks {
	public:
		virtual ~BLEClientCallbacks() {};
		virtual void OnConnect(BLEClientGeneric *pClient) = 0;
		virtual void OnDisconnect(BLEClientGeneric *pClient) = 0;
};

#endif // CONFIG_BT_ENABLED
#endif /* DRIVERS_BLECLIENT_H_ */
