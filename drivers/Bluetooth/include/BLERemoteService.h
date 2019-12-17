/*
 *    BLERemoteService.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEREMOTESERVICE_H_
#define DRIVERS_BLEREMOTESERVICE_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <map>

#include "BLEClientGeneric.h"
#include "BLERemoteCharacteristic.h"
#include "BLEUUID.h"
#include "FreeRTOSWrapper.h"

class BLEClientGeneric;
class BLERemoteCharacteristic;

/**
 * @brief A model of a remote %BLE service.
 */
class BLERemoteService {
public:
	virtual ~BLERemoteService();

	// Public methods
	BLERemoteCharacteristic*	GetCharacteristic(const char* uuid);	  // Get the specified characteristic reference.
	BLERemoteCharacteristic*	GetCharacteristic(BLEUUID uuid);       // Get the specified characteristic reference.
	BLERemoteCharacteristic*	GetCharacteristic(uint16_t uuid);      // Get the specified characteristic reference.
	std::map<std::string, BLERemoteCharacteristic*>* GetCharacteristics();
	std::map<uint16_t, BLERemoteCharacteristic*>* GetCharacteristicsByHandle();  // Get the characteristics map.
	void GetCharacteristics(std::map<uint16_t, BLERemoteCharacteristic*>* pCharacteristicMap);

	BLEClientGeneric*			GetClient(void);                                           // Get a reference to the client associated with this service.
	uint16_t					GetHandle();                                               // Get the handle of this service.
	BLEUUID						GetUUID(void);                                             // Get the UUID of this service.
	string						GetValue(BLEUUID characteristicUuid);                      // Get the value of a characteristic.
	void 						SetValue(BLEUUID characteristicUuid, string value);   // Set the value of a characteristic.
	string						ToString(void);

private:
	// Private constructor ... never meant to be created by a user application.
	BLERemoteService(esp_gatt_id_t srvcId, BLEClientGeneric *pClient, uint16_t startHandle, uint16_t endHandle);

	// Friends
	friend class BLEClientGeneric;
	friend class BLERemoteCharacteristic;

	// Private methods
	void                RetrieveCharacteristics(void);   // Retrieve the characteristics from the BLE Server.
	esp_gatt_id_t*      GetSrvcId(void);
	uint16_t            GetStartHandle();                // Get the start handle for this service.
	uint16_t            GetEndHandle();                  // Get the end handle for this service.

	void                GattClientEventHandler(
		esp_gattc_cb_event_t      event,
		esp_gatt_if_t             gattc_if,
		esp_ble_gattc_cb_param_t* evtParam);

	void                RemoveCharacteristics();

	// Properties

	// We maintain a map of characteristics owned by this service keyed by a string representation of the UUID.
	std::map<std::string, BLERemoteCharacteristic*> m_characteristicMap;

	// We maintain a map of characteristics owned by this service keyed by a handle.
	std::map<uint16_t, BLERemoteCharacteristic*> m_characteristicMapByHandle;

	bool						m_haveCharacteristics; // Have we previously obtained the characteristics.
	BLEClientGeneric*		m_pClient;
	FreeRTOS::Semaphore	m_semaphoreGetCharEvt = FreeRTOS::Semaphore("GetCharEvt");
	esp_gatt_id_t			m_srvcId;
	BLEUUID					m_uuid;             // The UUID of this service.
	uint16_t					m_startHandle;      // The starting handle of this service.
	uint16_t					m_endHandle;        // The ending handle of this service.
}; // BLERemoteService

#endif /* CONFIG_BT_ENABLED */
#endif /* COMPONENTS_CPP_UTILS_BLEREMOTESERVICE_H_ */
