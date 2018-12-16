/*
 *    BLEDevice.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_bt.h>            // ESP32 BLE
#include <esp_bt_device.h>     // ESP32 BLE
#include <esp_bt_main.h>       // ESP32 BLE
#include <esp_gap_ble_api.h>   // ESP32 BLE
#include <esp_gatts_api.h>     // ESP32 BLE
#include <esp_gattc_api.h>     // ESP32 BLE
#include <esp_err.h>           // ESP32 ESP-IDF
#include <esp_log.h>           // ESP32 ESP-IDF
#include <map>                 // Part of C++ Standard library
#include <sstream>             // Part of C++ Standard library
#include <iomanip>             // Part of C++ Standard library

#include "BLEDevice.h"
#include "include/BLEClientGeneric.h"
#include "BLEUtils.h"
#include "GeneralUtils.h"

#include "Memory.h"

static char tag[] = "BLEDevice";

/**
 * Singletons for the BLEDevice.
 */
BLEServerGeneric* 	BLEDevice::m_pServer			= nullptr;
BLEScan*   			BLEDevice::m_pScan				= nullptr;
BLEClientGeneric*	BLEDevice::m_pClient			= nullptr;
bool				BLEDevice::IsInitialized          = false;  // Have we been initialized?

/**
 * @brief Initialize the %BLE environment.
 * @param deviceName The device name of the device.
 */
void BLEDevice::Init(std::string deviceName) {
	if(!IsInitialized) {
		IsInitialized = true;   // Set the initialization flag to ensure we are only initialized once.

		NVS::Init();

#ifndef CLASSIC_BT_ENABLED
		::esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#endif

		esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
		esp_err_t errRc = ::esp_bt_controller_init(&bt_cfg);

		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bt_controller_init: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

#ifndef CLASSIC_BT_ENABLED
		errRc = ::esp_bt_controller_enable(ESP_BT_MODE_BLE);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bt_controller_enable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}
#else
		errRc = ::esp_bt_controller_enable(ESP_BT_MODE_BTDM);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bt_controller_enable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}
#endif

		errRc = ::esp_bluedroid_init();
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bluedroid_init: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_bluedroid_enable();
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bluedroid_enable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_ble_gap_register_callback(BLEDevice::gapEventHandler);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_ble_gap_register_callback: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_ble_gattc_register_callback(BLEDevice::gattClientEventHandler);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_ble_gattc_register_callback: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_ble_gatts_register_callback(BLEDevice::gattServerEventHandler);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_ble_gatts_register_callback: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_ble_gap_set_device_name(deviceName.c_str());
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_ble_gap_set_device_name: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		};

		esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
		errRc = ::esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_ble_gap_set_security_param: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		};
	}
} // init

/**
 * @brief Deinitialize the %BLE environment.
 */

void BLEDevice::Deinit() {
	if(IsInitialized) {
		IsInitialized = false;

		esp_err_t errRc = ::esp_bluedroid_disable();
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bluedroid_disable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_bluedroid_deinit();
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bluedroid_deinit: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_bt_controller_disable();
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bt_controller_disable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_bt_controller_deinit();
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_bt_controller_deinit: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

	}
} // init


/**
 * @brief Create a new instance of a client.
 * @return A new instance of the client.
 */
/* STATIC */ BLEClientGeneric* BLEDevice::CreateClient() {
	m_pClient = new BLEClientGeneric();
	return m_pClient;
} // createClient


/**
 * @brief Create a new instance of a server.
 * @return A new instance of the server.
 */
/* STATIC */ BLEServerGeneric* BLEDevice::CreateServer() {
	ESP_LOGD(tag, ">> createServer");
	m_pServer = new BLEServerGeneric();
	m_pServer->CreateApp(0);
	ESP_LOGD(tag, "<< createServer");
	return m_pServer;
} // createServer


/**
 * @brief Handle GATT server events.
 *
 * @param [in] event The event that has been newly received.
 * @param [in] gatts_if The connection to the GATT interface.
 * @param [in] param Parameters for the event.
 */
/* STATIC */ void BLEDevice::gattServerEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	ESP_LOGD(tag, "gattServerEventHandler [esp_gatt_if: %d] ... %s", gatts_if, BLEUtils::gattServerEventTypeToString(event).c_str());
	BLEUtils::dumpGattServerEvent(event, gatts_if, param);

	if (BLEDevice::m_pServer != nullptr) {
		BLEDevice::m_pServer->HandleGATTServerEvent(event, gatts_if, param);
	}
} // gattServerEventHandler


/**
 * @brief Handle GATT client events.
 *
 * Handler for the GATT client events.
 *
 * @param [in] event
 * @param [in] gattc_if
 * @param [in] param
 */
/* STATIC */ void BLEDevice::gattClientEventHandler(
	esp_gattc_cb_event_t      event,
	esp_gatt_if_t             gattc_if,
	esp_ble_gattc_cb_param_t* param) {

	ESP_LOGD(tag, "gattClientEventHandler [esp_gatt_if: %d] ... %s",
		gattc_if, BLEUtils::gattClientEventTypeToString(event).c_str());
	BLEUtils::dumpGattClientEvent(event, gattc_if, param);
/*
	switch(event) {
		default: {
			break;
		}
	} // switch
	*/

	// If we have a client registered, call it.
	if (BLEDevice::m_pClient != nullptr) {
		BLEDevice::m_pClient->gattClientEventHandler(event, gattc_if, param);
	}

} // gattClientEventHandler


/**
 * @brief Handle GAP events.
 */
/* STATIC */ void BLEDevice::gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {

	BLEUtils::dumpGapEvent(event, param);

	switch(event) {
		case ESP_GAP_BLE_SEC_REQ_EVT: {
			esp_err_t errRc = ::esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
			if (errRc != ESP_OK) {
				ESP_LOGE(tag, "esp_ble_gap_security_rsp: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			}
			break;
		}

		default: {
			break;
		}
	} // switch

	if (BLEDevice::m_pServer != nullptr) {
		BLEDevice::m_pServer->HandleGAPEvent(event, param);
	}

	if (BLEDevice::m_pClient != nullptr) {
		BLEDevice::m_pClient->handleGAPEvent(event, param);
	}

	if (BLEDevice::m_pScan != nullptr) {
		BLEDevice::GetScan()->HandleGAPEvent(event, param);
	}
} // gapEventHandler


/**
 * @brief Get the BLE device address.
 * @return The BLE device address.
 */
/* STATIC*/ BLEAddress BLEDevice::GetAddress() {
	const uint8_t* bdAddr = esp_bt_dev_get_address();
	esp_bd_addr_t addr;
	memcpy(addr, bdAddr, sizeof(addr));
	return BLEAddress(addr);
} // getAddress


/**
 * @brief Retrieve the Scan object that we use for scanning.
 * @return The scanning object reference.  This is a singleton object.  The caller should not
 * try and release/delete it.
 */
/* STATIC */ BLEScan* BLEDevice::GetScan() {
	//ESP_LOGD(tag, ">> getScan");
	if (m_pScan == nullptr) {
		m_pScan = new BLEScan();
		//ESP_LOGD(tag, " - creating a new scan object");
	}
	//ESP_LOGD(tag, "<< getScan: Returning object at 0x%x", (uint32_t)m_pScan);
	return m_pScan;
} // getScan


/**
 * @brief Get the value of a characteristic of a service on a remote device.
 * @param [in] bdAddress
 * @param [in] serviceUUID
 * @param [in] characteristicUUID
 */
/* STATIC */ std::string BLEDevice::GetValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID) {
	ESP_LOGD(tag, ">> getValue: bdAddress: %s, serviceUUID: %s, characteristicUUID: %s", bdAddress.ToString().c_str(), serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());
	BLEClientGeneric *pClient = CreateClient();
	pClient->Connect(bdAddress);
	std::string ret = pClient->getValue(serviceUUID, characteristicUUID);
	pClient->Disconnect();
	ESP_LOGD(tag, "<< getValue");
	return ret;
} // getValue



/**
 * @brief Set the transmission power.
 * The power level can be one of:
 * * ESP_PWR_LVL_N14
 * * ESP_PWR_LVL_N11
 * * ESP_PWR_LVL_N8
 * * ESP_PWR_LVL_N5
 * * ESP_PWR_LVL_N2
 * * ESP_PWR_LVL_P1
 * * ESP_PWR_LVL_P4
 * * ESP_PWR_LVL_P7
 * @param [in] powerLevel.
 */
void BLEDevice::SetPower(esp_power_level_t powerLevel, esp_ble_power_type_t powerType ) {
	ESP_LOGD(tag, ">> setPower: %d", powerLevel);
	esp_err_t errRc = ::esp_ble_tx_power_set(powerType, powerLevel);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_tx_power_set: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
	};
	ESP_LOGD(tag, "<< setPower");
} // setPower


/**
 * @brief Set the value of a characteristic of a service on a remote device.
 * @param [in] bdAddress
 * @param [in] serviceUUID
 * @param [in] characteristicUUID
 */
/* STATIC */ void BLEDevice::SetValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID, std::string value) {
	ESP_LOGD(tag, ">> setValue: bdAddress: %s, serviceUUID: %s, characteristicUUID: %s", bdAddress.ToString().c_str(), serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());
	BLEClientGeneric *pClient = CreateClient();
	pClient->Connect(bdAddress);
	pClient->setValue(serviceUUID, characteristicUUID, value);
	pClient->Disconnect();
} // setValue


/**
 * @brief Return a string representation of the nature of this device.
 * @return A string representation of the nature of this device.
 */
/* STATIC */ std::string BLEDevice::toString() {
	std::ostringstream oss;
	oss << "BD Address: " << GetAddress().ToString();
	return oss.str();
} // toString


/**
 * @brief Add an entry to the BLE white list.
 * @param [in] address The address to add to the white list.
 */
void BLEDevice::WhiteListAdd(BLEAddress address) {
	ESP_LOGD(tag, ">> whiteListAdd: %s", address.ToString().c_str());
	esp_err_t errRc = esp_ble_gap_update_whitelist(true, *address.GetNative());  // True to add an entry.
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gap_update_whitelist: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
	}
	ESP_LOGD(tag, "<< whiteListAdd");
} // whiteListAdd

/**
 * @brief Remove an entry from the BLE white list.
 * @param [in] address The address to remove from the white list.
 */
void BLEDevice::WhiteListRemove(BLEAddress address) {
	ESP_LOGD(tag, ">> whiteListRemove: %s", address.ToString().c_str());
	esp_err_t errRc = esp_ble_gap_update_whitelist(false, *address.GetNative());  // False to remove an entry.
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gap_update_whitelist: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
	}
	ESP_LOGD(tag, "<< whiteListRemove");
} // whiteListRemove

#endif // CONFIG_BT_ENABLED
