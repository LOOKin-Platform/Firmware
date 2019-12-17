/*
 *    BLEServerGeneric.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLESERVER_H_
#define DRIVERS_BLESERVER_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_gatts_api.h>

#include <string>
#include <string.h>

#include "BLEUUID.h"
#include "BLEAdvertising.h"
#include "BLECharacteristic.h"
#include "BLEService.h"
#include "BLESecurity.h"
#include "FreeRTOSWrapper.h"
#include "BLEAddress.h"

class BLEServerCallbacks;

typedef struct {
	void 						*peer_device;	// peer device BLEClient or BLEServer - maybe its better to have 2 structures or union here
	bool 						connected;		// do we need it?
	uint16_t 					mtu;			// every peer device negotiate own mtu
} conn_status_t;

/**
 * @brief A data structure that manages the %BLE servers owned by a BLE server.
 */
class BLEServiceMap {
	public:
		BLEService*				GetByHandle(uint16_t handle);
		BLEService* 			GetByUUID(const char* uuid);
		BLEService* 			GetByUUID(BLEUUID uuid, uint8_t inst_id = 0);
		void        			HandleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
		void        			SetByHandle(uint16_t handle, BLEService* service);
		void        			SetByUUID(const char* uuid, BLEService* service);
		void        			SetByUUID(BLEUUID uuid, BLEService* service);
		string 					ToString();
		BLEService* 			GetFirst();
		BLEService* 			GetNext();
		void 					RemoveService(BLEService *service);
		int 					GetRegisteredServiceCount();

	private:
		map<uint16_t,BLEService*>	m_handleMap;
		map<BLEService*, string>	m_uuidMap;
		std::map<BLEService*, std::string>::iterator m_iterator;

};

/**
 * @brief The model of a %BLE server.
 */
class BLEServerGeneric {
	public:
		uint32_t				GetConnectedCount();
		BLEService*				CreateService(const char* uuid);
		BLEService*				CreateService(BLEUUID uuid, uint32_t numHandles=15, uint8_t inst_id=0);
		BLEAdvertising*			GetAdvertising();
		void					SetCallbacks(BLEServerCallbacks* pCallbacks);
		void					StartAdvertising();
		void 					RemoveService(BLEService* service);
		BLEService* 			GetServiceByUUID(const char* uuid);
		BLEService* 			GetServiceByUUID(BLEUUID uuid);
		bool 					Connect(BLEAddress address);
		void 					Disconnect(uint16_t connId);
		uint16_t				m_appId;
		void					UpdateConnParams(esp_bd_addr_t remote_bda, uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout);

		/* multi connection support */
		std::map<uint16_t, conn_status_t>
								GetPeerDevices(bool client);
		void 					AddPeerDevice(void* peer, bool is_client, uint16_t conn_id);
		void 					RemovePeerDevice(uint16_t conn_id, bool client);
		BLEServerGeneric* 		GetServerByConnId(uint16_t conn_id);
		void 					UpdatePeerMTU(uint16_t connId, uint16_t mtu);
		uint16_t 				GetPeerMTU(uint16_t conn_id);
		uint16_t        		GetConnId();

	private:
		BLEServerGeneric();
		friend class BLEService;
		friend class BLECharacteristic;
		friend class BLEDevice;

		esp_ble_adv_data_t  	m_adv_data;
		// BLEAdvertising      m_bleAdvertising;
		uint16_t				m_connId;
		uint32_t            	m_connectedCount;
		uint16_t            	m_gatts_if;
			std::map<uint16_t, conn_status_t> m_connectedServersMap;

		FreeRTOS::Semaphore 	m_semaphoreRegisterAppEvt 	= FreeRTOS::Semaphore("RegisterAppEvt");
		FreeRTOS::Semaphore 	m_semaphoreCreateEvt 		= FreeRTOS::Semaphore("CreateEvt");
		FreeRTOS::Semaphore 	m_semaphoreOpenEvt   		= FreeRTOS::Semaphore("OpenEvt");
		BLEServiceMap       	m_serviceMap;
		BLEServerCallbacks* 	m_pServerCallbacks = nullptr;

		void            		CreateApp(uint16_t appId);
		uint16_t        		GetGattsIf();
		void            		HandleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
		void            		RegisterApp(uint16_t);
}; // BLEServer

/**
 * @brief Callbacks associated with the operation of a %BLE server.
 */
class BLEServerCallbacks {
	public:
		virtual ~BLEServerCallbacks() {};
		/**
		 * @brief Handle a new client connection.
		 *
		 * When a new client connects, we are invoked.
		 *
		 * @param [in] pServer A reference to the %BLE server that received the client connection.
		 */
		virtual void OnConnect(BLEServerGeneric* pServer);
		virtual void OnConnect(BLEServerGeneric* pServer, esp_ble_gatts_cb_param_t *param);

		/**
		 * @brief Handle an existing client disconnection.
		 *
		 * When an existing client disconnects, we are invoked.
		 *
		 * @param [in] pServer A reference to the %BLE server that received the existing client disconnection.
		 */
		virtual void OnDisconnect(BLEServerGeneric* pServer);
}; // BLEServerCallbacks



#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLESERVER_H_ */
