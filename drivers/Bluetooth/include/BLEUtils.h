/*
 *    BLEUtils.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEUTILS_H_
#define DRIVERS_BLEUTILS_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_gattc_api.h>   // ESP32 BLE
#include <esp_gatts_api.h>   // ESP32 BLE
#include <esp_gap_ble_api.h> // ESP32 BLE
#include <string>
#include "BLEClientGeneric.h"

/**
 * @brief A set of general %BLE utilities.
 */
class BLEUtils {
public:
	static const char*			AddressTypeToString(esp_ble_addr_type_t type);
	static string				AdFlagsToString(uint8_t adFlags);
	static const char*			AdvTypeToString(uint8_t advType);
	static char*				BuildHexData(uint8_t* target, uint8_t* source, uint8_t length);
	static string        		BuildPrintData(uint8_t* source, size_t length);
	static string        		CharacteristicPropertiesToString(esp_gatt_char_prop_t prop);
	static const char*			DevTypeToString(esp_bt_dev_type_t type);

	static esp_gatt_id_t		BuildGattId(esp_bt_uuid_t uuid, uint8_t inst_id=0);
	static esp_gatt_srvc_id_t	BuildGattSrvcId(esp_gatt_id_t gattId, bool is_primary=true);

	static void 				DumpGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
	static void 				DumpGattClientEvent(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* evtParam);
	static void					DumpGattServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* evtParam);

	static const char* 			EventTypeToString(esp_ble_evt_type_t eventType);
	static BLEClientGeneric*	FindByAddress(BLEAddress address);
	static BLEClientGeneric*	FindByConnId(uint16_t conn_id);
	static const char* 			GapEventToString(uint32_t eventType);

	static string 				GattCharacteristicUUIDToString(uint32_t characteristicUUID);
	static string 				GattClientEventTypeToString(esp_gattc_cb_event_t eventType);
	static string 				GattCloseReasonToString(esp_gatt_conn_reason_t reason);
	static string 				GattcServiceElementToString(esp_gattc_service_elem_t *pGATTCServiceElement);
	static string 				GattDescriptorUUIDToString(uint32_t descriptorUUID);
	static string 				GattServerEventTypeToString(esp_gatts_cb_event_t eventType);
	static string 				GattServiceIdToString(esp_gatt_srvc_id_t srvcId);
	static string 				GattServiceToString(uint32_t serviceId);
	static string 				GattStatusToString(esp_gatt_status_t status);
	static string 				GetMember(uint32_t memberId);
	static void        			RegisterByAddress(BLEAddress address, BLEClientGeneric* pDevice);
	static void        			RegisterByConnId(uint16_t conn_id, BLEClientGeneric* pDevice);
	static const char* 			SearchEventTypeToString(esp_gap_search_evt_t searchEvt);
};

#endif // CONFIG_BT_ENABLED
#endif /* DRIVERS_BLEUTILS_H_ */
