/*
 *    BLEService.cpp
 *    Bluetooth driver
 *
 */

// A service is identified by a UUID.  A service is also the container for one or more characteristics.

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_err.h>
#include <esp_gatts_api.h>
#include <esp_log.h>

#include <iomanip>
#include <sstream>
#include <string>

#include "BLEServerGeneric.h"
#include "BLEService.h"
#include "BLEUtils.h"
#include "GeneralUtils.h"

#include "esp_log.h"
static const char* tag = "BLEService"; // Tag for logging.


#define NULL_HANDLE (0xffff)


/**
 * @brief Construct an instance of the BLEService
 * @param [in] uuid The UUID of the service.
 * @param [in] numHandles The maximum number of handles associated with the service.
 */
BLEService::BLEService(const char* uuid, uint16_t numHandles) : BLEService(BLEUUID(uuid), numHandles) {
}


/**
 * @brief Construct an instance of the BLEService
 * @param [in] uuid The UUID of the service.
 * @param [in] numHandles The maximum number of handles associated with the service.
 */
BLEService::BLEService(BLEUUID uuid, uint16_t numHandles) {
	m_uuid      = uuid;
	m_handle    = NULL_HANDLE;
	m_pServer   = nullptr;
	//m_serializeMutex.setName("BLEService");
	m_lastCreatedCharacteristic = nullptr;
	m_numHandles = numHandles;
} // BLEService


/**
 * @brief Create the service.
 * Create the service.
 * @param [in] gatts_if The handle of the GATT server interface.
 * @return N/A.
 */

void BLEService::ExecuteCreate(BLEServerGeneric* pServer) {
	ESP_LOGD(tag, ">> executeCreate() - Creating service (esp_ble_gatts_create_service) service uuid: %s", GetUUID().ToString().c_str());
	m_pServer          = pServer;
	m_semaphoreCreateEvt.Take("executeCreate"); // Take the mutex and release at event ESP_GATTS_CREATE_EVT

	esp_gatt_srvc_id_t srvc_id;
	srvc_id.is_primary = true;
	srvc_id.id.inst_id = m_instId;
	srvc_id.id.uuid    = *m_uuid.GetNative();
	esp_err_t errRc = ::esp_ble_gatts_create_service(GetServer()->GetGattsIf(), &srvc_id, m_numHandles); // The maximum number of handles associated with the service.

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gatts_create_service: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}

	m_semaphoreCreateEvt.Wait("executeCreate");
	ESP_LOGD(tag, "<< executeCreate");
} // executeCreate


/**
 * @brief Delete the service.
 * Delete the service.
 * @return N/A.
 */

void BLEService::ExecuteDelete() {
	ESP_LOGD(tag, ">> executeDelete()");
	m_semaphoreDeleteEvt.Take("executeDelete"); // Take the mutex and release at event ESP_GATTS_DELETE_EVT

	esp_err_t errRc = ::esp_ble_gatts_delete_service(GetHandle());

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gatts_delete_service: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}

	m_semaphoreDeleteEvt.Wait("executeDelete");
	ESP_LOGD(tag, "<< executeDelete");
} // executeDelete


/**
 * @brief Dump details of this BLE GATT service.
 * @return N/A.
 */
void BLEService::Dump() {
	ESP_LOGD(tag, "Service: uuid:%s, handle: 0x%.2x",
		m_uuid.ToString().c_str(),
		m_handle);
	ESP_LOGD(tag, "Characteristics:\n%s", m_characteristicMap.ToString().c_str());
} // dump


/**
 * @brief Get the UUID of the service.
 * @return the UUID of the service.
 */
BLEUUID BLEService::GetUUID() {
	return m_uuid;
} // getUUID


/**
 * @brief Start the service.
 * Here we wish to start the service which means that we will respond to partner requests about it.
 * Starting a service also means that we can create the corresponding characteristics.
 * @return Start the service.
 */
void BLEService::Start() {
// We ask the BLE runtime to start the service and then create each of the characteristics.
// We start the service through its local handle which was returned in the ESP_GATTS_CREATE_EVT event
// obtained as a result of calling esp_ble_gatts_create_service().
//
	ESP_LOGD(tag, ">> start(): Starting service (esp_ble_gatts_start_service): %s", ToString().c_str());
	if (m_handle == NULL_HANDLE) {
		ESP_LOGE(tag, "<< !!! We attempted to start a service but don't know its handle!");
		return;
	}

	BLECharacteristic *pCharacteristic = m_characteristicMap.GetFirst();

	while (pCharacteristic != nullptr) {
		m_lastCreatedCharacteristic = pCharacteristic;
		pCharacteristic->ExecuteCreate(this);

		pCharacteristic = m_characteristicMap.GetNext();
	}
	// Start each of the characteristics ... these are found in the m_characteristicMap.

	m_semaphoreStartEvt.Take("start");
	esp_err_t errRc = ::esp_ble_gatts_start_service(m_handle);

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "<< esp_ble_gatts_start_service: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}
	m_semaphoreStartEvt.Wait("start");

	ESP_LOGD(tag, "<< start()");
} // start


/**
 * @brief Stop the service.
 */
void BLEService::Stop() {
// We ask the BLE runtime to start the service and then create each of the characteristics.
// We start the service through its local handle which was returned in the ESP_GATTS_CREATE_EVT event
// obtained as a result of calling esp_ble_gatts_create_service().
	ESP_LOGD(tag, ">> stop(): Stopping service (esp_ble_gatts_stop_service): %s", ToString().c_str());
	if (m_handle == NULL_HANDLE) {
		ESP_LOGE(tag, "<< !!! We attempted to stop a service but don't know its handle!");
		return;
	}

	m_semaphoreStopEvt.Take("stop");
	esp_err_t errRc = ::esp_ble_gatts_stop_service(m_handle);

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "<< esp_ble_gatts_stop_service: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}
	m_semaphoreStopEvt.Wait("stop");

	ESP_LOGD(tag, "<< stop()");
} // start


/**
 * @brief Set the handle associated with this service.
 * @param [in] handle The handle associated with the service.
 */
void BLEService::SetHandle(uint16_t handle) {
	ESP_LOGD(tag, ">> setHandle - Handle=0x%.2x, service UUID=%s)", handle, GetUUID().ToString().c_str());
	if (m_handle != NULL_HANDLE) {
		ESP_LOGE(tag, "!!! Handle is already set %.2x", m_handle);
		return;
	}
	m_handle = handle;
	ESP_LOGD(tag, "<< setHandle");
} // setHandle


/**
 * @brief Get the handle associated with this service.
 * @return The handle associated with this service.
 */
uint16_t BLEService::GetHandle() {
	return m_handle;
} // getHandle


/**
 * @brief Add a characteristic to the service.
 * @param [in] pCharacteristic A pointer to the characteristic to be added.
 */
void BLEService::AddCharacteristic(BLECharacteristic* pCharacteristic) {
	// We maintain a mapping of characteristics owned by this service.  These are managed by the
	// BLECharacteristicMap class instance found in m_characteristicMap.  We add the characteristic
	// to the map and then ask the service to add the characteristic at the BLE level (ESP-IDF).

	ESP_LOGD(tag, ">> addCharacteristic()");
	ESP_LOGD(tag, "Adding characteristic: uuid=%s to service: %s",
		pCharacteristic->GetUUID().ToString().c_str(),
		ToString().c_str());

	// Check that we don't add the same characteristic twice.
	if (m_characteristicMap.GetByUUID(pCharacteristic->GetUUID()) != nullptr) {
		ESP_LOGW(tag, "<< Adding a new characteristic with the same UUID as a previous one");
		//return;
	}

	// Remember this characteristic in our map of characteristics.  At this point, we can lookup by UUID
	// but not by handle.  The handle is allocated to us on the ESP_GATTS_ADD_CHAR_EVT.
	m_characteristicMap.SetByUUID(pCharacteristic, pCharacteristic->GetUUID());

	ESP_LOGD(tag, "<< addCharacteristic()");
} // addCharacteristic


/**
 * @brief Create a new BLE Characteristic associated with this service.
 * @param [in] uuid - The UUID of the characteristic.
 * @param [in] properties - The properties of the characteristic.
 * @return The new BLE characteristic.
 */
BLECharacteristic* BLEService::CreateCharacteristic(const char* uuid, uint32_t properties) {
	return CreateCharacteristic(BLEUUID(uuid), properties);
}
	

/**
 * @brief Create a new BLE Characteristic associated with this service.
 * @param [in] uuid - The UUID of the characteristic.
 * @param [in] properties - The properties of the characteristic.
 * @return The new BLE characteristic.
 */
BLECharacteristic* BLEService::CreateCharacteristic(BLEUUID uuid, uint32_t properties) {
	BLECharacteristic* pCharacteristic = new BLECharacteristic(uuid, properties);
	AddCharacteristic(pCharacteristic);
	return pCharacteristic;
} // createCharacteristic


/**
 * @brief Handle a GATTS server event.
 */
void BLEService::HandleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	switch (event) {
		// ESP_GATTS_ADD_CHAR_EVT - Indicate that a characteristic was added to the service.
		// add_char:
		// - esp_gatt_status_t status
		// - uint16_t attr_handle
		// - uint16_t service_handle
		// - esp_bt_uuid_t char_uuid

		// If we have reached the correct service, then locate the characteristic and remember the handle
		// for that characteristic.
		case ESP_GATTS_ADD_CHAR_EVT: {
			if (m_handle == param->add_char.service_handle) {
				BLECharacteristic *pCharacteristic = GetLastCreatedCharacteristic();
				if (pCharacteristic == nullptr) {
					ESP_LOGE(tag, "Expected to find characteristic with UUID: %s, but didnt!",
							BLEUUID(param->add_char.char_uuid).ToString().c_str());
					Dump();
					break;
				}
				pCharacteristic->SetHandle(param->add_char.attr_handle);
				m_characteristicMap.SetByHandle(param->add_char.attr_handle, pCharacteristic);
				break;
			} // Reached the correct service.
			break;
		} // ESP_GATTS_ADD_CHAR_EVT


		// ESP_GATTS_START_EVT
		//
		// start:
		// esp_gatt_status_t status
		// uint16_t service_handle
		case ESP_GATTS_START_EVT: {
			if (param->start.service_handle == GetHandle()) {
				m_semaphoreStartEvt.Give();
			}
			break;
		} // ESP_GATTS_START_EVT

		// ESP_GATTS_STOP_EVT
		//
		// stop:
		// esp_gatt_status_t status
		// uint16_t service_handle
		//
		case ESP_GATTS_STOP_EVT: {
			if (param->stop.service_handle == GetHandle()) {
				m_semaphoreStopEvt.Give();
			}
			break;
		} // ESP_GATTS_STOP_EVT


		// ESP_GATTS_CREATE_EVT
		// Called when a new service is registered as having been created.
		//
		// create:
		// * esp_gatt_status_t status
		// * uint16_t service_handle
		// * esp_gatt_srvc_id_t service_id
		// * - esp_gatt_id id
		// *   - esp_bt_uuid uuid
		// *   - uint8_t inst_id
		// * - bool is_primary
		//
		case ESP_GATTS_CREATE_EVT: {
			if (GetUUID().Equals(BLEUUID(param->create.service_id.id.uuid)) && m_instId == param->create.service_id.id.inst_id) {
				SetHandle(param->create.service_handle);
				m_semaphoreCreateEvt.Give();
			}
			break;
		} // ESP_GATTS_CREATE_EVT


		// ESP_GATTS_DELETE_EVT
		// Called when a service is deleted.
		//
		// delete:
		// * esp_gatt_status_t status
		// * uint16_t service_handle
		//
		case ESP_GATTS_DELETE_EVT: {
			if (param->del.service_handle == GetHandle()) {
				m_semaphoreDeleteEvt.Give();
			}
			break;
		} // ESP_GATTS_DELETE_EVT

		default:
			break;
	} // Switch

	// Invoke the GATTS handler in each of the associated characteristics.
	m_characteristicMap.HandleGATTServerEvent(event, gatts_if, param);
} // handleGATTServerEvent


BLECharacteristic* BLEService::GetCharacteristic(const char* uuid) {
	return GetCharacteristic(BLEUUID(uuid));
}


BLECharacteristic* BLEService::GetCharacteristic(BLEUUID uuid) {
	return m_characteristicMap.GetByUUID(uuid);
}


/**
 * @brief Return a string representation of this service.
 * A service is defined by:
 * * Its UUID
 * * Its handle
 * @return A string representation of this service.
 */
std::string BLEService::ToString() {
	std::stringstream stringStream;
	stringStream << "UUID: " << GetUUID().ToString() <<
		", handle: 0x" << std::hex << std::setfill('0') << std::setw(2) << GetHandle();
	return stringStream.str();
} // toString


/**
 * @brief Get the last created characteristic.
 * It is lamentable that this function has to exist.  It returns the last created characteristic.
 * We need this because the descriptor API is built around the notion that a new descriptor, when created,
 * is associated with the last characteristics created and we need that information.
 * @return The last created characteristic.
 */
BLECharacteristic* BLEService::GetLastCreatedCharacteristic() {
	return m_lastCreatedCharacteristic;
} // getLastCreatedCharacteristic


/**
 * @brief Get the BLE server associated with this service.
 * @return The BLEServer associated with this service.
 */
BLEServerGeneric* BLEService::GetServer() {
	return m_pServer;
} // getServer

#endif // CONFIG_BT_ENABLED
