/*
 *    BLEService.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLESERVICE_H_
#define DRIVERS_BLESERVICE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <esp_gatts_api.h>

#include "BLECharacteristic.h"
#include "BLEServerGeneric.h"
#include "BLEUUID.h"
#include "FreeRTOSWrapper.h"

class BLEServerGeneric;

/**
 * @brief A data mapping used to manage the set of %BLE characteristics known to the server.
 */
class BLECharacteristicMap {
	public:
		void SetByUUID(BLECharacteristic* pCharacteristic, const char* uuid);
		void SetByUUID(BLECharacteristic* pCharacteristic, BLEUUID uuid);
		void SetByHandle(uint16_t handle, BLECharacteristic* pCharacteristic);

		BLECharacteristic*	GetByUUID(const char* uuid);
		BLECharacteristic*	GetByUUID(BLEUUID uuid);
		BLECharacteristic*	GetByHandle(uint16_t handle);
		BLECharacteristic*	GetFirst();
		BLECharacteristic*	GetNext();
		string 				ToString();

		void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
	private:
		map<BLECharacteristic*, string> 			m_uuidMap;
		map<uint16_t, BLECharacteristic*> 			m_handleMap;
		map<BLECharacteristic*, string>::iterator 	m_iterator;
};


/**
 * @brief The model of a %BLE service.
 *
 */
class BLEService {
	public:
		void				AddCharacteristic(BLECharacteristic* pCharacteristic);
		BLECharacteristic*	CreateCharacteristic(const char* uuid, uint32_t properties);
		BLECharacteristic*	CreateCharacteristic(BLEUUID uuid, uint32_t properties);
		void				Dump();
		void				ExecuteCreate(BLEServerGeneric* pServer);
		BLECharacteristic*	GetCharacteristic(const char* uuid);
		BLECharacteristic*	GetCharacteristic(BLEUUID uuid);
		BLEUUID				GetUUID();
		BLEServerGeneric*	GetServer();
		void				Start();
		void				Stop();
		string				ToString();
		uint16_t			GetHandle();

		~BLEService();

	private:
		BLEService(const char* uuid, uint32_t numHandles);
		BLEService(BLEUUID uuid, uint32_t numHandles);

		friend class BLEServerGeneric;
		friend class BLEServiceMap;
		friend class BLEDescriptor;
		friend class BLECharacteristic;
		friend class BLEDevice;

		BLECharacteristicMap	m_characteristicMap;
		uint16_t				m_handle;
		BLECharacteristic*		m_lastCreatedCharacteristic;
		BLEServerGeneric*		m_pServer;
		BLEUUID					m_uuid;

		FreeRTOS::Semaphore		m_semaphoreCreateEvt 	= FreeRTOS::Semaphore("CreateEvt");
		FreeRTOS::Semaphore		m_semaphoreStartEvt 	= FreeRTOS::Semaphore("StartEvt");
		FreeRTOS::Semaphore		m_semaphoreStopEvt  	= FreeRTOS::Semaphore("StopEvt");

		uint32_t				m_numHandles;

		BLECharacteristic* 		getLastCreatedCharacteristic();

		void					handleGATTServerEvent(esp_gatts_cb_event_t event,esp_gatt_if_t gatts_if,esp_ble_gatts_cb_param_t* param);
		void					setHandle(uint16_t handle);
		//void               setService(esp_gatt_srvc_id_t srvc_id);
}; // BLEService


#endif // CONFIG_BT_ENABLED
#endif /* DRIVERS_BLESERVICE_H_ */
