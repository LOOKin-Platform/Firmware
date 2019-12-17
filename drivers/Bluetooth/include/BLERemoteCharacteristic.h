/*
 *    BLERemoteCharacteristic.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEREMOTECHARACTERISTIC_H_
#define DRIVERS_BLEREMOTECHARACTERISTIC_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <string>

#include <esp_gattc_api.h>

#include "BLERemoteService.h"
#include "BLERemoteDescriptor.h"
#include "BLEUUID.h"
#include "FreeRTOSWrapper.h"

class BLERemoteService;
class BLERemoteDescriptor;
typedef void (*notify_callback)(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

/**
 * @brief A model of a remote %BLE characteristic.
 */
class BLERemoteCharacteristic {
public:
	~BLERemoteCharacteristic();

	// Public member functions
	bool        			CanBroadcast();
	bool        			CanIndicate();
	bool        			CanNotify();
	bool        			CanRead();
	bool        			CanWrite();
	bool        			CanWriteNoResponse();
	BLERemoteDescriptor* 	GetDescriptor(BLEUUID uuid);
	std::map<std::string, BLERemoteDescriptor*>*
							GetDescriptors();
	uint16_t    			GetHandle();
	BLEUUID     			GetUUID();
	std::string 			ReadValue();
	uint8_t     			ReadUInt8();
	uint16_t    			ReadUInt16();
	uint32_t    			ReadUInt32();
	void        			RegisterForNotify(notify_callback _callback, bool notifications = true);
	bool        			WriteValue(uint8_t* data, size_t length, bool response = false);
	bool        			WriteValue(std::string newValue, bool response = false);
	bool        			WriteValue(uint8_t newValue, bool response = false);
	std::string 			ToString();
	uint8_t*				ReadRawData();
	BLERemoteService* 		GetRemoteService();

private:
	BLERemoteCharacteristic(uint16_t handle, BLEUUID uuid, esp_gatt_char_prop_t charProp, BLERemoteService* pRemoteService);
	friend class BLEClientGeneric;
	friend class BLERemoteService;
	friend class BLERemoteDescriptor;

	// Private member functions
	void gattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* evtParam);

	void              		RemoveDescriptors();
	void              		RetrieveDescriptors();

	// Private properties
	BLEUUID              	m_uuid;
	esp_gatt_char_prop_t 	m_charProp;
	uint16_t             	m_handle;
	BLERemoteService*    	m_pRemoteService;
	FreeRTOS::Semaphore  	m_semaphoreReadCharEvt      = FreeRTOS::Semaphore("ReadCharEvt");
	FreeRTOS::Semaphore  	m_semaphoreRegForNotifyEvt  = FreeRTOS::Semaphore("RegForNotifyEvt");
	FreeRTOS::Semaphore  	m_semaphoreWriteCharEvt     = FreeRTOS::Semaphore("WriteCharEvt");
	std::string          	m_value;
	uint8_t 			 	*m_rawData = nullptr;
	notify_callback		 	m_notifyCallback = nullptr;

	// We maintain a map of descriptors owned by this characteristic keyed by a string representation of the UUID.
	std::map<std::string, BLERemoteDescriptor*> m_descriptorMap;
}; // BLERemoteCharacteristic
#endif /* CONFIG_BT_ENABLED */
#endif /* COMPONENTS_CPP_UTILS_BLEREMOTECHARACTERISTIC_H_ */
