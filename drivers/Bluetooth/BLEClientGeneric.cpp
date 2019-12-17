/*
 *    BLEClientGenericGeneric.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include "BLEClientGeneric.h"
#include "BLEUtils.h"
#include "BLEService.h"
#include "GeneralUtils.h"
#include "BLEDevice.h"

#include <string>
#include <sstream>
#include <unordered_set>


static const char* tag = "BLEClientGeneric";

/*
 * Design
 * ------
 * When we perform a searchService() requests, we are asking the BLE server to return each of the services
 * that it exposes.  For each service, we received an ESP_GATTC_SEARCH_RES_EVT event which contains details
 * of the exposed service including its UUID.
 *
 * The objects we will invent for a BLEClientGeneric will be as follows:
 * * BLERemoteService - A model of a remote service.
 * * BLERemoteCharacteristic - A model of a remote characteristic
 * * BLERemoteDescriptor - A model of a remote descriptor.
 *
 * Since there is a hierarchical relationship here, we will have the idea that from a BLERemoteService will own
 * zero or more remote characteristics and a BLERemoteCharacteristic will own zero or more remote BLEDescriptors.
 *
 * We will assume that a BLERemoteService contains a map that maps BLEUUIDs to the set of owned characteristics
 * and that a BLECharacteristic contains a map that maps BLEUUIDs to the set of owned descriptors.
 *
 *
 */

BLEClientGeneric::BLEClientGeneric() {
	m_pClientCallbacks = nullptr;
	m_conn_id          = ESP_GATT_IF_NONE;
	m_gattc_if         = ESP_GATT_IF_NONE;
	m_haveServices     = false;
	m_isConnected      = false;  // Initially, we are flagged as not connected.


	m_appId = BLEDevice::m_appId++;
	m_appId = m_appId%100;
	BLEDevice::AddPeerDevice(this, true, m_appId);
	m_semaphoreRegEvt.Take("connect");

	esp_err_t errRc = ::esp_ble_gattc_app_register(m_appId);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_app_register: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}

	m_semaphoreRegEvt.Wait("connect");

} // BLEClientGeneric


/**
 * @brief Destructor.
 */
BLEClientGeneric::~BLEClientGeneric() {
	// We may have allocated service references associated with this client.  Before we are finished
	// with the client, we must release resources.
	ClearServices();
	esp_ble_gattc_app_unregister(m_gattc_if);
	BLEDevice::RemovePeerDevice(m_appId, true);
	if(m_deleteCallbacks)
		delete m_pClientCallbacks;

} // ~BLEClientGeneric


/**
 * @brief Clear any existing services.
 *
 */
void BLEClientGeneric::ClearServices() {
	ESP_LOGD(tag, ">> clearServices");
	// Delete all the services.
	for (auto &myPair : m_servicesMap) {
	   delete myPair.second;
	}
	m_servicesMap.clear();
	m_servicesMapByInstID.clear();
	m_haveServices = false;
	ESP_LOGD(tag, "<< clearServices");
} // clearServices

/**
 * Add overloaded function to ease connect to peer device with not public address
 */
bool BLEClientGeneric::Connect(BLEAdvertisedDevice* device) {
	BLEAddress address =  device->GetAddress();
	esp_ble_addr_type_t type = device->GetAddressType();
	return Connect(address, type);
}

/**
 * @brief Connect to the partner (BLE Server).
 * @param [in] address The address of the partner.
 * @return True on success.
 */
bool BLEClientGeneric::Connect(BLEAddress address, esp_ble_addr_type_t type) {
	ESP_LOGD(tag, ">> connect(%s)", address.ToString().c_str());

	ClearServices();
	m_peerAddress = address;

	// Perform the open connection request against the target BLE Server.
	m_semaphoreOpenEvt.Take("connect");
	esp_err_t errRc = ::esp_ble_gattc_open(
		m_gattc_if,
		*GetPeerAddress().GetNative(), // address
		type,          // Note: This was added on 2018-04-03 when the latest ESP-IDF was detected to have changed the signature.
		1                              // direct connection <-- maybe needs to be changed in case of direct indirect connection???
	);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_open: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return false;
	}

	uint32_t rc = m_semaphoreOpenEvt.Wait("connect");   // Wait for the connection to complete.
	ESP_LOGD(tag, "<< connect(), rc=%d", rc==ESP_GATT_OK);
	return rc == ESP_GATT_OK;
} // connect


/**
 * @brief Disconnect from the peer.
 * @return N/A.
 */
void BLEClientGeneric::Disconnect() {
	ESP_LOGD(tag, ">> disconnect()");
	// ESP_LOGW(__func__, "gattIf: %d, connId: %d", getGattcIf(), getConnId());
	esp_err_t errRc = ::esp_ble_gattc_close(GetGattcIf(), GetConnId());
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_close: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return;
	}
	ESP_LOGD(tag, "<< disconnect()");
} // disconnect


/**
 * @brief Handle GATT Client events
 */
void BLEClientGeneric::GattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* evtParam) {

	ESP_LOGD(tag, "gattClientEventHandler [esp_gatt_if: %d] ... %s",
		gattc_if, BLEUtils::GattClientEventTypeToString(event).c_str());

	// Execute handler code based on the type of event received.
	switch(event) {

		case ESP_GATTC_SRVC_CHG_EVT:
			if(m_gattc_if != gattc_if)
				break;

			ESP_LOGI(tag, "SERVICE CHANGED");
			break;

		case ESP_GATTC_CLOSE_EVT:
			break;

		//
		// ESP_GATTC_DISCONNECT_EVT
		//
		// disconnect:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		// - esp_bd_addr_t     remote_bda
		case ESP_GATTC_DISCONNECT_EVT: {
			ESP_LOGE(__func__, "disconnect event, reason: %d, connId: %d, my connId: %d, my IF: %d, gattc_if: %d", (int)evtParam->disconnect.reason, evtParam->disconnect.conn_id, GetConnId(), GetGattcIf(), gattc_if);
			if(m_gattc_if != gattc_if)
				break;
			m_semaphoreOpenEvt.Give(evtParam->disconnect.reason);
			if(!m_isConnected)
				break;
			// If we receive a disconnect event, set the class flag that indicates that we are
			// no longer connected.
			esp_ble_gattc_close(m_gattc_if, m_conn_id);
			m_isConnected = false;
			if (m_pClientCallbacks != nullptr) {
				m_pClientCallbacks->OnDisconnect(this);
			}
			break;
		} // ESP_GATTC_DISCONNECT_EVT

		//
		// ESP_GATTC_OPEN_EVT
		//
		// open:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		// - esp_bd_addr_t     remote_bda
		//
		case ESP_GATTC_OPEN_EVT: {
			if(m_gattc_if != gattc_if)
				break;
			m_conn_id = evtParam->open.conn_id;
			if (m_pClientCallbacks != nullptr) {
				m_pClientCallbacks->OnConnect(this);
			}
			if (evtParam->open.status == ESP_GATT_OK) {
				m_isConnected = true;   // Flag us as connected.
				m_mtu = evtParam->open.mtu;
			}
			m_semaphoreOpenEvt.Give(evtParam->open.status);
			break;
		} // ESP_GATTC_OPEN_EVT


		//
		// ESP_GATTC_REG_EVT
		//
		// reg:
		// esp_gatt_status_t status
		// uint16_t          app_id
		//
		case ESP_GATTC_REG_EVT: {
			if(m_appId == evtParam->reg.app_id){
				ESP_LOGI(__func__, "register app id: %d, %d, gattc_if: %d", m_appId, evtParam->reg.app_id, gattc_if);
				m_gattc_if = gattc_if;
				m_semaphoreRegEvt.Give();
			}
			break;
		} // ESP_GATTC_REG_EVT

		case ESP_GATTC_CFG_MTU_EVT:
			if(evtParam->cfg_mtu.status != ESP_GATT_OK) {
				ESP_LOGE(tag,"Config mtu failed");
			}
			else
				m_mtu = evtParam->cfg_mtu.mtu;
			break;

		case ESP_GATTC_CONNECT_EVT: {
			if(m_gattc_if != gattc_if)
				break;
			m_conn_id = evtParam->connect.conn_id;
			BLEDevice::UpdatePeerDevice(this, true, m_gattc_if);
			esp_err_t errRc = esp_ble_gattc_send_mtu_req(gattc_if, evtParam->connect.conn_id);
			if (errRc != ESP_OK) {
				ESP_LOGE(tag, "esp_ble_gattc_send_mtu_req: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
			}
#ifdef CONFIG_BLE_SMP_ENABLE   // Check that BLE SMP (security) is configured in make menuconfig
			if(BLEDevice::m_securityLevel){
				esp_ble_set_encryption(evtParam->connect.remote_bda, BLEDevice::m_securityLevel);
			}
#endif	// CONFIG_BLE_SMP_ENABLE
			break;
		} // ESP_GATTC_CONNECT_EVT

		//
		// ESP_GATTC_SEARCH_CMPL_EVT
		//
		// search_cmpl:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		//
		case ESP_GATTC_SEARCH_CMPL_EVT: {
			if(m_gattc_if != gattc_if)
				break;
			if (evtParam->search_cmpl.status != ESP_GATT_OK){
				ESP_LOGE(tag, "search service failed, error status = %x", evtParam->search_cmpl.status);
			}
#ifndef ARDUINO_ARCH_ESP32
// commented out just for now to keep backward compatibility
			// if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
			// 	ESP_LOGI(tag, "Get service information from remote device");
			// } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
			// 	ESP_LOGI(tag, "Get service information from flash");
			// } else {
			// 	ESP_LOGI(tag, "unknown service source");
			// }
#endif
			m_semaphoreSearchCmplEvt.Give(evtParam->search_cmpl.status);
			break;
		} // ESP_GATTC_SEARCH_CMPL_EVT


		//
		// ESP_GATTC_SEARCH_RES_EVT
		//
		// search_res:
		// - uint16_t      conn_id
		// - uint16_t      start_handle
		// - uint16_t      end_handle
		// - esp_gatt_id_t srvc_id
		//
		case ESP_GATTC_SEARCH_RES_EVT: {
			if(m_gattc_if != gattc_if)
				break;

			BLEUUID uuid = BLEUUID(evtParam->search_res.srvc_id);
			BLERemoteService* pRemoteService = new BLERemoteService(
				evtParam->search_res.srvc_id,
				this,
				evtParam->search_res.start_handle,
				evtParam->search_res.end_handle
			);
			m_servicesMap.insert(std::pair<std::string, BLERemoteService*>(uuid.ToString(), pRemoteService));
			m_servicesMapByInstID.insert(std::pair<BLERemoteService *, uint16_t>(pRemoteService, evtParam->search_res.srvc_id.inst_id));
			break;
		} // ESP_GATTC_SEARCH_RES_EVT


		default: {
			break;
		}
	} // Switch

	// Pass the request on to all services.
	for (auto &myPair : m_servicesMap) {
	   myPair.second->GattClientEventHandler(event, gattc_if, evtParam);
	}

} // gattClientEventHandler


uint16_t BLEClientGeneric::GetConnId() {
	return m_conn_id;
} // getConnId



esp_gatt_if_t BLEClientGeneric::GetGattcIf() {
	return m_gattc_if;
} // getGattcIf


/**
 * @brief Retrieve the address of the peer.
 *
 * Returns the Bluetooth device address of the %BLE peer to which this client is connected.
 */
BLEAddress BLEClientGeneric::GetPeerAddress() {
	return m_peerAddress;
} // getAddress


/**
 * @brief Ask the BLE server for the RSSI value.
 * @return The RSSI value.
 */
int BLEClientGeneric::GetRssi() {
	ESP_LOGD(tag, ">> getRssi()");
	if (!IsConnected()) {
		ESP_LOGD(tag, "<< getRssi(): Not connected");
		return 0;
	}
	// We make the API call to read the RSSI value which is an asynchronous operation.  We expect to receive
	// an ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT to indicate completion.
	//
	m_semaphoreRssiCmplEvt.Take("getRssi");
	esp_err_t rc = ::esp_ble_gap_read_rssi(*GetPeerAddress().GetNative());
	if (rc != ESP_OK) {
		ESP_LOGE(tag, "<< getRssi: esp_ble_gap_read_rssi: rc=%d %s", rc, GeneralUtils::ErrorToString(rc));
		return 0;
	}
	int rssiValue = m_semaphoreRssiCmplEvt.Wait("getRssi");
	ESP_LOGD(tag, "<< getRssi(): %d", rssiValue);
	return rssiValue;
} // getRssi


/**
 * @brief Get the service BLE Remote Service instance corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A reference to the Service or nullptr if don't know about it.
 */
BLERemoteService* BLEClientGeneric::GetService(const char* uuid) {
    return GetService(BLEUUID(uuid));
} // getService


/**
 * @brief Get the service object corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A reference to the Service or nullptr if don't know about it.
 * @throws BLEUuidNotFound
 */
BLERemoteService* BLEClientGeneric::GetService(BLEUUID uuid) {
	ESP_LOGD(tag, ">> getService: uuid: %s", uuid.ToString().c_str());
// Design
// ------
// We wish to retrieve the service given its UUID.  It is possible that we have not yet asked the
// device what services it has in which case we have nothing to match against.  If we have not
// asked the device about its services, then we do that now.  Once we get the results we can then
// examine the services map to see if it has the service we are looking for.
	if (!m_haveServices) {
		GetServices();
	}
	std::string uuidStr = uuid.ToString();
	for (auto &myPair : m_servicesMap) {
		if (myPair.first == uuidStr) {
			ESP_LOGD(tag, "<< getService: found the service with uuid: %s", uuid.ToString().c_str());
			return myPair.second;
		}
	} // End of each of the services.
	ESP_LOGD(tag, "<< getService: not found");
	return nullptr;
} // getService


/**
 * @brief Ask the remote %BLE server for its services.
 * A %BLE Server exposes a set of services for its partners.  Here we ask the server for its set of
 * services and wait until we have received them all.
 * @return N/A
 */
map<string, BLERemoteService*>* BLEClientGeneric::GetServices() {
/*
 * Design
 * ------
 * We invoke esp_ble_gattc_search_service.  This will request a list of the service exposed by the
 * peer BLE partner to be returned as events.  Each event will be an an instance of ESP_GATTC_SEARCH_RES_EVT
 * and will culminate with an ESP_GATTC_SEARCH_CMPL_EVT when all have been received.
 */
	ESP_LOGD(tag, ">> getServices");
	// TODO implement retrieving services from cache
	ClearServices(); // Clear any services that may exist.

	esp_err_t errRc = esp_ble_gattc_search_service(GetGattcIf(), GetConnId(), NULL );

	m_semaphoreSearchCmplEvt.Take("getServices");
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_search_service: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		return &m_servicesMap;
	}
	// If sucessfull, remember that we now have services.
	m_haveServices = (m_semaphoreSearchCmplEvt.Wait("getServices") == 0);
	ESP_LOGD(tag, "<< getServices");
	return &m_servicesMap;
} // getServices


/**
 * @brief Get the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to read.
 * @throws BLEUuidNotFound
 */
std::string BLEClientGeneric::GetValue(BLEUUID serviceUUID, BLEUUID characteristicUUID) {
	ESP_LOGD(tag, ">> getValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.ToString().c_str(), characteristicUUID.ToString().c_str());
	std::string ret = GetService(serviceUUID)->GetCharacteristic(characteristicUUID)->ReadValue();
	ESP_LOGD(tag, "<<getValue");
	return ret;
} // getValue


/**
 * @brief Handle a received GAP event.
 *
 * @param [in] event
 * @param [in] param
 */
void BLEClientGeneric::HandleGAPEvent(esp_gap_ble_cb_event_t  event, esp_ble_gap_cb_param_t* param) {
	ESP_LOGD(tag, "BLEClientGeneric ... handling GAP event!");
	switch (event) {
		//
		// ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT
		//
		// read_rssi_cmpl
		// - esp_bt_status_t status
		// - int8_t rssi
		// - esp_bd_addr_t remote_addr
		//
		case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: {
			m_semaphoreRssiCmplEvt.Give((uint32_t) param->read_rssi_cmpl.rssi);
			break;
		} // ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT

		default:
			break;
	}
} // handleGAPEvent


/**
 * @brief Are we connected to a partner?
 * @return True if we are connected and false if we are not connected.
 */
bool BLEClientGeneric::IsConnected() {
	return m_isConnected;
} // isConnected




/**
 * @brief Set the callbacks that will be invoked.
 */
void BLEClientGeneric::SetClientCallbacks(BLEClientCallbacks* pClientCallbacks, bool deleteCallbacks) {
	m_pClientCallbacks = pClientCallbacks;
	m_deleteCallbacks = deleteCallbacks;
} // setClientCallbacks


/**
 * @brief Set the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to write.
 * @throws BLEUuidNotFound
 */
void BLEClientGeneric::SetValue(BLEUUID serviceUUID, BLEUUID characteristicUUID, std::string value) {
	ESP_LOGD(tag, ">> setValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.ToString().c_str(), characteristicUUID.ToString().c_str());
	GetService(serviceUUID)->GetCharacteristic(characteristicUUID)->WriteValue(value);
	ESP_LOGD(tag, "<< setValue");
} // setValue

uint16_t BLEClientGeneric::GetMTU() {
	return m_mtu;
}

/**
 * @brief Return a string representation of this client.
 * @return A string representation of this client.
 */
std::string BLEClientGeneric::ToString() {
	std::ostringstream ss;
	ss << "peer address: " << m_peerAddress.ToString();
	ss << "\nServices:\n";
	for (auto &myPair : m_servicesMap) {
		ss << myPair.second->ToString() << "\n";
	  // myPair.second is the value
	}
	return ss.str();
} // toString


#endif // CONFIG_BT_ENABLED
