/*
 *    BLEDescriptor.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <sstream>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_err.h>
#include "BLEService.h"
#include "BLEDescriptor.h"
#include "GeneralUtils.h"

static char tag[] = "BLEDescriptor";

#define NULL_HANDLE (0xffff)

/**
 * @brief BLEDescriptor constructor.
 */
BLEDescriptor::BLEDescriptor(const char* uuid, uint16_t len) : BLEDescriptor(BLEUUID(uuid), len) {
}	

/**
 * @brief BLEDescriptor constructor.
 */
BLEDescriptor::BLEDescriptor(BLEUUID uuid, uint16_t max_len) {
	m_bleUUID            = uuid;
	m_value.attr_len     = 0;                                         // Initial length is 0.
	m_value.attr_max_len = max_len;                     // Maximum length of the data.
	m_handle             = NULL_HANDLE;                               // Handle is initially unknown.
	m_pCharacteristic    = nullptr;                                   // No initial characteristic.
	m_pCallback          = nullptr;                                   // No initial callback.

	m_value.attr_value   = (uint8_t*) malloc(max_len);  // Allocate storage for the value.
} // BLEDescriptor


/**
 * @brief BLEDescriptor destructor.
 */
BLEDescriptor::~BLEDescriptor() {
	free(m_value.attr_value);   // Release the storage we created in the constructor.
} // ~BLEDescriptor


/**
 * @brief Execute the creation of the descriptor with the BLE runtime in ESP.
 * @param [in] pCharacteristic The characteristic to which to register this descriptor.
 */
void BLEDescriptor::ExecuteCreate(BLECharacteristic* pCharacteristic) {
	ESP_LOGD(tag, ">> executeCreate(): %s", ToString().c_str());

	if (m_handle != NULL_HANDLE) {
		ESP_LOGE(tag, "Descriptor already has a handle.");
		return;
	}

	m_pCharacteristic = pCharacteristic; // Save the characteristic associated with this service.

	esp_attr_control_t control;
	control.auto_rsp = ESP_GATT_AUTO_RSP;
	m_semaphoreCreateEvt.Take("executeCreate");

	esp_err_t errRc = ::esp_ble_gatts_add_char_descr(
			pCharacteristic->GetService()->GetHandle(),
			GetUUID().GetNative(),
			(esp_gatt_perm_t)m_permissions, &m_value, &control);

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "<< esp_ble_gatts_add_char_descr: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}

	m_semaphoreCreateEvt.Wait("executeCreate");
	ESP_LOGD(tag, "<< executeCreate");
} // executeCreate


/**
 * @brief Get the BLE handle for this descriptor.
 * @return The handle for this descriptor.
 */
uint16_t BLEDescriptor::GetHandle() {
	return m_handle;
} // getHandle


/**
 * @brief Get the length of the value of this descriptor.
 * @return The length (in bytes) of the value of this descriptor.
 */
size_t BLEDescriptor::GetLength() {
	return m_value.attr_len;
} // getLength


/**
 * @brief Get the UUID of the descriptor.
 */
BLEUUID BLEDescriptor::GetUUID() {
	return m_bleUUID;
} // getUUID



/**
 * @brief Get the value of this descriptor.
 * @return A pointer to the value of this descriptor.
 */
uint8_t* BLEDescriptor::GetValue() {
	return m_value.attr_value;
} // getValue


/**
 * @brief Handle GATT server events for the descripttor.
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 */
void BLEDescriptor::HandleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {
	switch (event) {
		// ESP_GATTS_ADD_CHAR_DESCR_EVT
		//
		// add_char_descr:
		// - esp_gatt_status_t status
		// - uint16_t          attr_handle
		// - uint16_t          service_handle
		// - esp_bt_uuid_t     char_uuid
		case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
			if (m_pCharacteristic != nullptr &&
					m_bleUUID.Equals(BLEUUID(param->add_char_descr.descr_uuid)) &&
					m_pCharacteristic->GetService()->GetHandle() == param->add_char_descr.service_handle &&
					m_pCharacteristic == m_pCharacteristic->GetService()->GetLastCreatedCharacteristic()) {
				SetHandle(param->add_char_descr.attr_handle);
				m_semaphoreCreateEvt.Give();
			}
			break;
		} // ESP_GATTS_ADD_CHAR_DESCR_EVT

		// ESP_GATTS_WRITE_EVT - A request to write the value of a descriptor has arrived.
		//
		// write:
		// - uint16_t conn_id
		// - uint16_t trans_id
		// - esp_bd_addr_t bda
		// - uint16_t handle
		// - uint16_t offset
		// - bool need_rsp
		// - bool is_prep
		// - uint16_t len
		// - uint8_t *value
		case ESP_GATTS_WRITE_EVT: {
			if (param->write.handle == m_handle) {
				SetValue(param->write.value, param->write.len);   // Set the value of the descriptor.

				if (m_pCallback != nullptr) {   // We have completed the write, if there is a user supplied callback handler, invoke it now.
					m_pCallback->OnWrite(this);   // Invoke the onWrite callback handler.
				}
			}  // End of ... this is our handle.

			break;
		} // ESP_GATTS_WRITE_EVT

		// ESP_GATTS_READ_EVT - A request to read the value of a descriptor has arrived.
		//
		// read:
		// - uint16_t conn_id
		// - uint32_t trans_id
		// - esp_bd_addr_t bda
		// - uint16_t handle
		// - uint16_t offset
		// - bool is_long
		// - bool need_rsp
		//
		case ESP_GATTS_READ_EVT: {
			if (param->read.handle == m_handle) {  // If this event is for this descriptor ... process it

				if (m_pCallback != nullptr) {   // If we have a user supplied callback, invoke it now.
					m_pCallback->OnRead(this);    // Invoke the onRead callback method in the callback handler.
				}

			} // End of this is our handle
			break;
		} // ESP_GATTS_READ_EVT

		default:
			break;
	} // switch event
} // handleGATTServerEvent


/**
 * @brief Set the callback handlers for this descriptor.
 * @param [in] pCallbacks An instance of a callback structure used to define any callbacks for the descriptor.
 */
void BLEDescriptor::SetCallbacks(BLEDescriptorCallbacks* pCallback) {
	ESP_LOGD(tag, ">> setCallbacks: 0x%x", (uint32_t) pCallback);
	m_pCallback = pCallback;
	ESP_LOGD(tag, "<< setCallbacks");
} // setCallbacks


/**
 * @brief Set the handle of this descriptor.
 * Set the handle of this descriptor to be the supplied value.
 * @param [in] handle The handle to be associated with this descriptor.
 * @return N/A.
 */
void BLEDescriptor::SetHandle(uint16_t handle) {
	ESP_LOGD(tag, ">> setHandle(0x%.2x): Setting descriptor handle to be 0x%.2x", handle, handle);
	m_handle = handle;
	ESP_LOGD(tag, "<< setHandle()");
} // setHandle


/**
 * @brief Set the value of the descriptor.
 * @param [in] data The data to set for the descriptor.
 * @param [in] length The length of the data in bytes.
 */
void BLEDescriptor::SetValue(uint8_t* data, size_t length) {
	if (length > ESP_GATT_MAX_ATTR_LEN) {
		ESP_LOGE(tag, "Size %d too large, must be no bigger than %d", length, ESP_GATT_MAX_ATTR_LEN);
		return;
	}
	m_value.attr_len = length;
	memcpy(m_value.attr_value, data, length);
} // setValue


/**
 * @brief Set the value of the descriptor.
 * @param [in] value The value of the descriptor in string form.
 */
void BLEDescriptor::SetValue(std::string value) {
	SetValue((uint8_t*) value.data(), value.length());
} // setValue

void BLEDescriptor::SetAccessPermissions(esp_gatt_perm_t perm) {
	m_permissions = perm;
}

/**
 * @brief Return a string representation of the descriptor.
 * @return A string representation of the descriptor.
 */
std::string BLEDescriptor::ToString() {
	std::stringstream stringstream;
	stringstream << std::hex << std::setfill('0');
	stringstream << "UUID: " << m_bleUUID.ToString() + ", handle: 0x" << std::setw(2) << m_handle;
	return stringstream.str();
} // toString


BLEDescriptorCallbacks::~BLEDescriptorCallbacks() {}

/**
 * @brief Callback function to support a read request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 */
void BLEDescriptorCallbacks::OnRead(BLEDescriptor* pDescriptor) {
	ESP_LOGD("BLEDescriptorCallbacks", ">> onRead: default");
	ESP_LOGD("BLEDescriptorCallbacks", "<< onRead");
} // onRead


/**
 * @brief Callback function to support a write request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 */
void BLEDescriptorCallbacks::OnWrite(BLEDescriptor* pDescriptor) {
	ESP_LOGD("BLEDescriptorCallbacks", ">> onWrite: default");
	ESP_LOGD("BLEDescriptorCallbacks", "<< onWrite");
} // onWrite


#endif /* CONFIG_BT_ENABLED */
