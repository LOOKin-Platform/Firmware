/*
 *    BLECharacteristic.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLECHARACTERISTIC_H_
#define DRIVERS_BLECHARACTERISTIC_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <sstream>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include <string>
#include <map>

#include "BLEUUID.h"
#include "BLEDescriptor.h"
#include "BLEValue.h"

#include "FreeRTOSWrapper.h"

#include <esp_log.h>
#include <esp_err.h>
#include <esp_gatts_api.h>
#include <esp_gap_ble_api.h>

class BLEService;
class BLEDescriptor;
class BLECharacteristicCallbacks;

/**
 * @brief A management structure for %BLE descriptors.
 */
class BLEDescriptorMap {
	public:
		void 			SetByUUID(const char* uuid, BLEDescriptor *pDescriptor);
		void 			SetByUUID(BLEUUID uuid, BLEDescriptor *pDescriptor);
		void 			SetByHandle(uint16_t handle, BLEDescriptor *pDescriptor);
		BLEDescriptor* 	GetByUUID(const char* uuid);
		BLEDescriptor*	GetByUUID(BLEUUID uuid);
		BLEDescriptor*	GetByHandle(uint16_t handle);
		string			ToString();
		void			HandleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
		BLEDescriptor*	GetFirst();
		BLEDescriptor*	GetNext();

	private:
		map<BLEDescriptor *, string> 			m_uuidMap;
		map<uint16_t, BLEDescriptor *> 			m_handleMap;
		map<BLEDescriptor *, string>::iterator 	m_iterator;
};

/**
 * @brief The model of a %BLE Characteristic.
 *
 * A %BLE Characteristic is an identified value container that manages a value.  It is exposed by a %BLE server and
 * can be read and written to by a %BLE client.
 */
class BLECharacteristic {
	public:
		BLECharacteristic(const char* uuid, uint32_t properties = 0);
		BLECharacteristic(BLEUUID uuid, uint32_t properties = 0);
		virtual ~BLECharacteristic();

		void 			AddDescriptor(BLEDescriptor* pDescriptor);
		BLEDescriptor*	GetDescriptorByUUID(const char* descriptorUUID);
		BLEDescriptor*	GetDescriptorByUUID(BLEUUID descriptorUUID);
		BLEUUID			GetUUID();
		string			GetValue();
		uint8_t*		GetData();

		void			Indicate();
		void			Notify(bool is_notification = true);
		void			SetBroadcastProperty(bool value);
		void			SetCallbacks		(BLECharacteristicCallbacks* pCallbacks);
		void			SetIndicateProperty	(bool value);
		void			SetNotifyProperty	(bool value);
		void			SetReadProperty		(bool value);
		void 			SetValue(uint8_t* data, size_t size);
		void 			SetValue(std::string value);
		void 			SetValue(uint16_t& data16);
		void 			SetValue(uint32_t& data32);
		void 			SetValue(int& data32);
		void 			SetValue(float& data32);
		void 			SetValue(double& data64);
		void			SetWriteProperty	(bool value);
		void			SetWriteNoResponseProperty(bool value);
		string			ToString();
		uint16_t		GetHandle();
		void			SetAccessPermissions	(esp_gatt_perm_t perm);

		static const uint32_t PROPERTY_READ 	= 1 << 0;
		static const uint32_t PROPERTY_WRITE 	= 1 << 1;
		static const uint32_t PROPERTY_NOTIFY 	= 1 << 2;
		static const uint32_t PROPERTY_BROADCAST= 1 << 3;
		static const uint32_t PROPERTY_INDICATE = 1 << 4;
		static const uint32_t PROPERTY_WRITE_NR = 1 << 5;

	private:
		friend class BLEServerGeneric;
		friend class BLEService;
		friend class BLEDescriptor;
		friend class BLECharacteristicMap;

		BLEUUID						m_bleUUID;
		BLEDescriptorMap			m_descriptorMap;
		uint16_t					m_handle;
		esp_gatt_char_prop_t		m_properties;
		BLECharacteristicCallbacks*	m_pCallbacks;
		BLEService*					m_pService;
		BLEValue					m_value;
		esp_gatt_perm_t				m_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
		bool						m_writeEvt = false;

		void HandleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

		void 						ExecuteCreate(BLEService* pService);
		esp_gatt_char_prop_t 		GetProperties();
		BLEService* 				GetService();
		void 						SetHandle(uint16_t handle);

		FreeRTOS::Semaphore m_semaphoreCreateEvt	= FreeRTOS::Semaphore("CreateEvt");
		FreeRTOS::Semaphore m_semaphoreConfEvt 		= FreeRTOS::Semaphore("ConfEvt");
};
// BLECharacteristic

/**
 * @brief Callbacks that can be associated with a %BLE characteristic to inform of events.
 *
 * When a server application creates a %BLE characteristic, we may wish to be informed when there is either
 * a read or write request to the characteristic's value.  An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class BLECharacteristicCallbacks {
	public:
		virtual ~BLECharacteristicCallbacks();
		virtual void OnRead(BLECharacteristic* pCharacteristic);
		virtual void OnWrite(BLECharacteristic* pCharacteristic);
};
#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLECHARACTERISTIC_H_ */
