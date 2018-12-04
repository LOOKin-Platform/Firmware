/*
 *    BLEClientGeneric.cpp
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
#include "include/BLEClientGeneric.h"
#include "BLEUtils.h"
#include "BLEService.h"
#include "GeneralUtils.h"
#include <string>
#include <sstream>
#include <unordered_set>

/*
 * Design
 * ------
 * When we perform a searchService() requests, we are asking the BLE server to return each of the services
 * that it exposes.  For each service, we received an ESP_GATTC_SEARCH_RES_EVT event which contains details
 * of the exposed service including its UUID.
 *
 * The objects we will invent for a BLEClient will be as follows:
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
static const char* tag = "BLEClient";

BLEClientGeneric::BLEClientGeneric() {
	m_pClientCallbacks = nullptr;
	m_conn_id          = 0;
	m_gattc_if         = 0;
	m_haveServices     = false;
	m_isConnected      = false;  // Initially, we are flagged as not connected.
} // BLEClient


/**
 * @brief Destructor.
 */
BLEClientGeneric::~BLEClientGeneric() {
	// We may have allocated service references associated with this client.  Before we are finished
	// with the client, we must release resources.
	for (auto &myPair : m_servicesMap) {
	   delete myPair.second;
	}
	m_servicesMap.clear();
} // ~BLEClient


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
	ESP_LOGD(tag, "<< clearServices");
} // clearServices


/**
 * @brief Connect to the partner (BLE Server).
 * @param [in] address The address of the partner.
 * @return True on success.
 */
bool BLEClientGeneric::Connect(BLEAddress address) {
	ESP_LOGD(tag, ">> connect(%s)", address.ToString().c_str());

	// We need the connection handle that we get from registering the application.  We register the app
	// and then block on its completion.  When the event has arrived, we will have the handle.
	m_semaphoreRegEvt.Take("connect");

	ClearServices(); // Delete any services that may exist.

	esp_err_t errRc = ::esp_ble_gattc_app_register(0);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_app_register: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return false;
	}

	m_semaphoreRegEvt.Wait("connect");

	m_peerAddress = address;

	// Perform the open connection request against the target BLE Server.
	m_semaphoreOpenEvt.Take("connect");
	errRc = ::esp_ble_gattc_open(
		getGattcIf(),
		*getPeerAddress().GetNative(), // address
		BLE_ADDR_TYPE_PUBLIC,
		1                              // direct connection
	);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_open: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
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
	esp_err_t errRc = ::esp_ble_gattc_close(getGattcIf(), getConnId());
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_close: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}
	esp_ble_gattc_app_unregister(getGattcIf());
	m_peerAddress = BLEAddress("00:00:00:00:00:00");
	ESP_LOGD(tag, "<< disconnect()");
} // disconnect


/**
 * @brief Handle GATT Client events
 */
void BLEClientGeneric::gattClientEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* evtParam) {

	// Execute handler code based on the type of event received.
	switch(event) {

		//
		// ESP_GATTC_DISCONNECT_EVT
		//
		// disconnect:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		// - esp_bd_addr_t     remote_bda
		case ESP_GATTC_DISCONNECT_EVT: {
				// If we receive a disconnect event, set the class flag that indicates that we are
				// no longer connected.
				if (m_pClientCallbacks != nullptr) {
					m_pClientCallbacks->onDisconnect(this);
				}
				m_isConnected = false;
				break;
		} // ESP_GATTC_DISCONNECT_EVT

		case ESP_GATTS_CONNECT_EVT: {
			//m_connId = param->connect.conn_id; // Save the connection id.
			if(m_securityLevel){
				esp_ble_set_encryption(evtParam->connect.remote_bda, m_securityLevel);
				//memcpy(m_remote_bda, param->connect.remote_bda, sizeof(m_remote_bda));
			}
			break;
		} // ESP_GATTS_CONNECT_EVT

		//
		// ESP_GATTC_OPEN_EVT
		//
		// open:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		// - esp_bd_addr_t     remote_bda
		// - uint16_t          mtu
		//
		case ESP_GATTC_OPEN_EVT: {
			m_conn_id = evtParam->open.conn_id;

			if (m_pClientCallbacks != nullptr) {
				m_pClientCallbacks->onConnect(this);
			}
			if (evtParam->open.status == ESP_GATT_OK) {
				m_isConnected = true;   // Flag us as connected.
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
			m_gattc_if = gattc_if;
			m_semaphoreRegEvt.Give();
			break;
		} // ESP_GATTC_REG_EVT


		//
		// ESP_GATTC_SEARCH_CMPL_EVT
		//
		// search_cmpl:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		//
		case ESP_GATTC_SEARCH_CMPL_EVT: {
			m_semaphoreSearchCmplEvt.Give();
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
			BLEUUID uuid = BLEUUID(evtParam->search_res.srvc_id);
			BLERemoteService* pRemoteService = new BLERemoteService(evtParam->search_res.srvc_id, this,
														evtParam->search_res.start_handle, evtParam->search_res.end_handle);

			m_servicesMap.insert(pair<string, BLERemoteService *>(uuid.toString(), pRemoteService));
			break;
		} // ESP_GATTC_SEARCH_RES_EVT


		default: {
			break;
		}
	} // Switch

	// Pass the request on to all services.
	for (auto &myPair : m_servicesMap) {
	   myPair.second->gattClientEventHandler(event, gattc_if, evtParam);
	}

} // gattClientEventHandler


uint16_t BLEClientGeneric::getConnId() {
	return m_conn_id;
} // getConnId



esp_gatt_if_t BLEClientGeneric::getGattcIf() {
	return m_gattc_if;
} // getGattcIf


/**
 * @brief Retrieve the address of the peer.
 *
 * Returns the Bluetooth device address of the %BLE peer to which this client is connected.
 */
BLEAddress BLEClientGeneric::getPeerAddress() {
	return m_peerAddress;
} // getAddress


/**
 * @brief Ask the BLE server for the RSSI value.
 * @return The RSSI value.
 */
int BLEClientGeneric::getRssi() {
	ESP_LOGD(tag, ">> getRssi()");
	if (!isConnected()) {
		ESP_LOGD(tag, "<< getRssi(): Not connected");
		return 0;
	}
	// We make the API call to read the RSSI value which is an asynchronous operation.  We expect to receive
	// an ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT to indicate completion.
	//
	m_semaphoreRssiCmplEvt.Take("getRssi");
	esp_err_t rc = ::esp_ble_gap_read_rssi(*getPeerAddress().GetNative());
	if (rc != ESP_OK) {
		ESP_LOGE(tag, "<< getRssi: esp_ble_gap_read_rssi: rc=%d %s", rc, GeneralUtils::errorToString(rc));
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
BLERemoteService* BLEClientGeneric::getService(const char* uuid) {
    return getService(BLEUUID(uuid));
} // getService


/**
 * @brief Get the service object corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A reference to the Service or nullptr if don't know about it.
 * @throws BLEUuidNotFound
 */
BLERemoteService* BLEClientGeneric::getService(BLEUUID uuid) {
	ESP_LOGD(tag, ">> getService: uuid: %s", uuid.toString().c_str());
	// Design
	// ------
	// We wish to retrieve the service given its UUID.  It is possible that we have not yet asked the
	// device what services it has in which case we have nothing to match against.  If we have not
	// asked the device about its services, then we do that now.  Once we get the results we can then
	// examine the services map to see if it has the service we are looking for.
	if (!m_haveServices) {
		getServices();
	}

	std::string uuidStr = uuid.toString();
	for (auto &myPair : m_servicesMap) {
		if (myPair.first == uuidStr) {
			ESP_LOGD(tag, "<< getService: found the service with uuid: %s", uuid.toString().c_str());
			return myPair.second;
		}
	} // End of each of the services.
	ESP_LOGD(tag, "<< getService: not found");
	return nullptr;
} // getService

/**
 * @brief Ask the remote %BLE server for its services.
 * A %BLE Server exposes a set of services for its partners. Here we ask the server for its set of
 * services and wait until we have received them all.
 * @param [in] uuid The UUID of the service which we need. Maybe nullptr if we need to scan all avaliable services
 *
 * @return N/A
 */
map<std::string, BLERemoteService*>* BLEClientGeneric::getServices(esp_bt_uuid_t *uuid) {
	/*
	 * Design
	 * ------
	 * We invoke esp_ble_gattc_search_service.  This will request a list of the service exposed by the
	 * peer BLE partner to be returned as events.  Each event will be an an instance of ESP_GATTC_SEARCH_RES_EVT
	 * and will culminate with an ESP_GATTC_SEARCH_CMPL_EVT when all have been received.
	 */
	ESP_LOGD(tag, ">> getServices");

	ClearServices(); // Clear any services that may exist.

	esp_err_t errRc = esp_ble_gattc_search_service(getGattcIf(), getConnId(), uuid); // Filter UUID
	m_semaphoreSearchCmplEvt.Take("getServices");
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_ble_gattc_search_service: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return &m_servicesMap;
	}
	m_semaphoreSearchCmplEvt.Wait("getServices");
	m_haveServices = true; // Remember that we now have services.
	ESP_LOGD(tag, "<< getServices");
	return &m_servicesMap;
} // getServices


/**
 * @brief Get the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to read.
 * @throws BLEUuidNotFound
 */
std::string BLEClientGeneric::getValue(BLEUUID serviceUUID, BLEUUID characteristicUUID) {
	ESP_LOGD(tag, ">> getValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());
	std::string ret = getService(serviceUUID)->GetCharacteristic(characteristicUUID)->readValue();
	ESP_LOGD(tag, "<< getValue");
	return ret;
} // getValue


/**
 * @brief Handle a received GAP event.
 *
 * @param [in] event
 * @param [in] param
 */
void BLEClientGeneric::handleGAPEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
	ESP_LOGD(tag, "BLEClient ... handling GAP event!");
	switch(event) {
		//
		// ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT
		//
		// read_rssi_cmpl
		// - esp_bt_status_t status
		// - int8_t rssi
		// - esp_bd_addr_t remote_addr
		//
		case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: {
			m_semaphoreRssiCmplEvt.Give((uint32_t)param->read_rssi_cmpl.rssi);
			break;
		} // ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT

	    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
	        ESP_LOGI(tag, "ESP_GAP_BLE_PASSKEY_REQ_EVT: ");
	       // esp_log_buffer_hex(tag, m_remote_bda, sizeof(m_remote_bda));
	    	assert(m_securityCallbacks!=nullptr);
	       // esp_ble_passkey_reply(m_remote_bda, true, m_securityCallbacks->onPassKeyRequest());
	        break;

		/*
		 * TODO should we add white/black list comparison?
		 */
	    case ESP_GAP_BLE_SEC_REQ_EVT:
	        /* send the positive(true) security response to the peer device to accept the security request.
	        If not accept the security request, should sent the security response with negative(false) accept value*/
	    	if(m_securityCallbacks!=nullptr)
	    		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, m_securityCallbacks->onSecurityRequest());
        	else
	        	esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, false);
        	break;
	        /*
	         *
	         */
	    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
	        ///show the passkey number to the user to input it in the peer deivce.
	    	if(m_securityCallbacks!=nullptr)
	    		m_securityCallbacks->onPassKeyNotify(param->ble_security.key_notif.passkey);
	        break;
	    case ESP_GAP_BLE_KEY_EVT:
	        //shows the ble key info share with peer device to the user.
	        //ESP_LOGI(tag, "key type = %s", BLESecurity::esp_key_type_to_str(param->ble_security.ble_key.key_type));
	        break;
	    case ESP_GAP_BLE_AUTH_CMPL_EVT:
	        if(m_securityCallbacks!=nullptr)
	        	m_securityCallbacks->onAuthenticationComplete(param->ble_security.auth_cmpl);
	        break;

		default:
			break;
	}
} // handleGAPEvent


/**
 * @brief Are we connected to a partner?
 * @return True if we are connected and false if we are not connected.
 */
bool BLEClientGeneric::isConnected() {
	return m_isConnected;
} // isConnected




/**
 * @brief Set the callbacks that will be invoked.
 */
void BLEClientGeneric::setClientCallbacks(BLEClientCallbacks* pClientCallbacks) {
	m_pClientCallbacks = pClientCallbacks;
} // setClientCallbacks


/**
 * @brief Set the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to write.
 * @throws BLEUuidNotFound
 */
void BLEClientGeneric::setValue(BLEUUID serviceUUID, BLEUUID characteristicUUID, std::string value) {
	ESP_LOGD(tag, ">> setValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());
	getService(serviceUUID)->GetCharacteristic(characteristicUUID)->WriteValue(value);
	ESP_LOGD(tag, "<< setValue");
} // setValue

void BLEClientGeneric::setEncryptionLevel(esp_ble_sec_act_t level) {
	m_securityLevel = level;
}

void BLEClientGeneric::setSecurityCallbacks(BLESecurityCallbacks* callbacks) {
	m_securityCallbacks = callbacks;
}

/**
 * @brief Return a string representation of this client.
 * @return A string representation of this client.
 */
std::string BLEClientGeneric::toString() {
	std::ostringstream ss;
	ss << "peer address: " << m_peerAddress.ToString();
	ss << "\nServices:\n";
	for (auto &myPair : m_servicesMap) {
		ss << myPair.second->toString() << "\n";
	  // myPair.second is the value
	}
	return ss.str();
} // toString


#endif // CONFIG_BT_ENABLED
