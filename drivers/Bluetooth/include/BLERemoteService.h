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

using namespace std;

/**
 * @brief A model of a remote %BLE service.
 */
class BLERemoteService {
	public:

		virtual ~BLERemoteService();

		// Public methods
		BLERemoteCharacteristic* 					getCharacteristic(const char* uuid);// Get the specified characteristic reference.
		BLERemoteCharacteristic* 					getCharacteristic(BLEUUID uuid); // Get the specified characteristic reference.
		BLERemoteCharacteristic* 					getCharacteristic(uint16_t uuid); // Get the specified characteristic reference.
		map<std::string, BLERemoteCharacteristic*>* getCharacteristics();
		void 										getCharacteristics(map<uint16_t, BLERemoteCharacteristic*>* pCharacteristicMap); // Get the characteristics map.

		BLEClientGeneric* 							getClient(void);// Get a reference to the client associated with this service.
		uint16_t 									getHandle(); 	// Get the handle of this service.
		BLEUUID 									getUUID(void);	// Get the UUID of this service.
		string 										getValue(BLEUUID characteristicUuid); // Get the value of a characteristic.
		void 										setValue(BLEUUID characteristicUuid, string value); // Set the value of a characteristic.
		string 										toString(void);

	private:
		// Private constructor ... never meant to be created by a user application.
		BLERemoteService(esp_gatt_id_t srvcId, BLEClientGeneric* pClient, uint16_t startHandle, uint16_t endHandle);

		// Friends
		friend class BLEClientGeneric;
		friend class BLERemoteCharacteristic;

		// Private methods
		void 			retrieveCharacteristics(void); 	// Retrieve the characteristics from the BLE Server.
		esp_gatt_id_t* 	getSrvcId(void);
		uint16_t 		getStartHandle();         		// Get the start handle for this service.
		uint16_t 		getEndHandle();             	// Get the end handle for this service.

		void gattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* evtParam);
		void removeCharacteristics();

		// Properties

		// We maintain a map of characteristics owned by this service keyed by a string representation of the UUID.
		map<string, BLERemoteCharacteristic *> m_characteristicMap;

		// We maintain a map of characteristics owned by this service keyed by a handle.
		map<uint16_t, BLERemoteCharacteristic *> m_characteristicMapByHandle;

		bool m_haveCharacteristics; // Have we previously obtained the characteristics.
		BLEClientGeneric* m_pClient;
		FreeRTOS::Semaphore m_semaphoreGetCharEvt = FreeRTOS::Semaphore("GetCharEvt");

		esp_gatt_id_t 	m_srvcId;
		BLEUUID 		m_uuid;             // The UUID of this service.
		uint16_t 		m_startHandle;      // The starting handle of this service.
		uint16_t 		m_endHandle;        // The ending handle of this service.
};
// BLERemoteService

#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLEREMOTESERVICE_H_ */
