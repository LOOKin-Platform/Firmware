/*
 *    BLEServerGeneric.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include "BLEDevice.h"
#include "include/BLEServerGeneric.h"
#include "BLEService.h"
#include "BLEUtils.h"
#include <string.h>
#include <string>
#include <unordered_set>

static const char* LOG_TAG = "BLEServer";

/**
 * @brief Construct a %BLE Server
 *
 * This class is not designed to be individually instantiated.  Instead one should create a server by asking
 * the BLEDevice class.
 */
BLEServerGeneric::BLEServerGeneric() {
	m_appId            = -1;
	m_gatts_if         = -1;
	m_connectedCount   = 0;
	m_connId           = -1;
	m_pServerCallbacks = nullptr;

	//createApp(0);
} // BLEServer

/**
 * @brief Destruct a %BLE Server
 *
 */
BLEServerGeneric::~BLEServerGeneric() {
	if (m_pServerCallbacks != nullptr)
		delete (m_pServerCallbacks);

	::esp_ble_gap_stop_advertising();
	::esp_ble_gatts_app_unregister(m_gatts_if);
}


void BLEServerGeneric::CreateApp(uint16_t appId) {
	m_appId = appId;
	RegisterApp();
} // createApp


/**
 * @brief Create a %BLE Service.
 *
 * With a %BLE server, we can host one or more services.  Invoking this function causes the creation of a definition
 * of a new service.  Every service must have a unique UUID.
 * @param [in] uuid The UUID of the new service.
 * @return A reference to the new service object.
 */
BLEService* BLEServerGeneric::CreateService(const char* uuid) {
	return CreateService(BLEUUID(uuid));
}


/**
 * @brief Create a %BLE Service.
 *
 * With a %BLE server, we can host one or more services.  Invoking this function causes the creation of a definition
 * of a new service.  Every service must have a unique UUID.
 * @param [in] uuid The UUID of the new service.
 * @param [in] numHandles The maximum number of handles associated with this service.
 * @return A reference to the new service object.
 */
BLEService* BLEServerGeneric::CreateService(BLEUUID uuid, uint32_t numHandles) {
	ESP_LOGD(LOG_TAG, ">> createService - %s", uuid.toString().c_str());
	m_semaphoreCreateEvt.Take("createService");

	// Check that a service with the supplied UUID does not already exist.
	if (m_serviceMap.GetByUUID(uuid) != nullptr) {
		ESP_LOGE(LOG_TAG, "<< Attempt to create a new service with uuid %s but a service with that UUID already exists.",
			uuid.toString().c_str());
		m_semaphoreCreateEvt.Give();
		return nullptr;
	}

	BLEService* pService = new BLEService(uuid, numHandles);
	m_serviceMap.SetByUUID(uuid, pService); // Save a reference to this service being on this server.
	pService->ExecuteCreate(this);          // Perform the API calls to actually create the service.

	m_semaphoreCreateEvt.Wait("createService");

	ESP_LOGD(LOG_TAG, "<< createService");
	return pService;
} // createService


/**
 * @brief Retrieve the advertising object that can be used to advertise the existence of the server.
 *
 * @return An advertising object.
 */
BLEAdvertising* BLEServerGeneric::GetAdvertising() {
	return &m_bleAdvertising;
}

uint16_t BLEServerGeneric::GetConnId() {
	return m_connId;
}


/**
 * @brief Return the number of connected clients.
 * @return The number of connected clients.
 */
uint32_t BLEServerGeneric::GetConnectedCount() {
	return m_connectedCount;
} // getConnectedCount


uint16_t BLEServerGeneric::GetGattsIf() {
	return m_gatts_if;
}

/**
 * @brief Handle a received GAP event.
 *
 * @param [in] event
 * @param [in] param
 */
void BLEServerGeneric::HandleGAPEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
	ESP_LOGD(LOG_TAG, "BLEServer ... handling GAP event!");
	switch(event) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
			/*
			esp_ble_adv_params_t adv_params;
			adv_params.adv_int_min       = 0x20;
			adv_params.adv_int_max       = 0x40;
			adv_params.adv_type          = ADV_TYPE_IND;
			adv_params.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
			adv_params.channel_map       = ADV_CHNL_ALL;
			adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
			ESP_LOGD(tag, "Starting advertising");
			esp_err_t errRc = ::esp_ble_gap_start_advertising(&adv_params);
			if (errRc != ESP_OK) {
				ESP_LOGE(tag, "esp_ble_gap_start_advertising: rc=%d %s", errRc, espToString(errRc));
				return;
			}
			*/
			break;
		}
	    case ESP_GAP_BLE_OOB_REQ_EVT:                                /* OOB request event */
	        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
	        break;
	    case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
	        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
	        break;
	    case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
	        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
	        break;
	    case ESP_GAP_BLE_NC_REQ_EVT:
	        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_NC_REQ_EVT");
	        break;
	    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
	        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT: ");
	        esp_log_buffer_hex(LOG_TAG, m_remote_bda, sizeof(m_remote_bda));
	    	assert(m_securityCallbacks!=nullptr);
	        esp_ble_passkey_reply(m_remote_bda, true, m_securityCallbacks->onPassKeyRequest());
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
	        ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_KEY_EVT");
	    	//ESP_LOGI(LOG_TAG, "key type = %s", BLESecurity::esp_key_type_to_str(param->ble_security.ble_key.key_type));
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
 * @brief Handle a GATT Server Event.
 *
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 *
 */
void BLEServerGeneric::HandleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {

	ESP_LOGD(LOG_TAG, ">> handleGATTServerEvent: %s",
		BLEUtils::gattServerEventTypeToString(event).c_str());

	// Invoke the handler for every Service we have.
	m_serviceMap.HandleGATTServerEvent(event, gatts_if, param);

	switch(event) {
		// ESP_GATTS_ADD_CHAR_EVT - Indicate that a characteristic was added to the service.
		// add_char:
		// - esp_gatt_status_t status
		// - uint16_t          attr_handle
		// - uint16_t          service_handle
		// - esp_bt_uuid_t     char_uuid
		//
		case ESP_GATTS_ADD_CHAR_EVT: {
			break;
		} // ESP_GATTS_ADD_CHAR_EVT


		// ESP_GATTS_CONNECT_EVT
		// connect:
		// - uint16_t      conn_id
		// - esp_bd_addr_t remote_bda
		// - bool          is_connected
		//
		case ESP_GATTS_CONNECT_EVT: {
			m_connId = param->connect.conn_id; // Save the connection id.

			if(m_securityLevel){
				esp_ble_set_encryption(param->connect.remote_bda, m_securityLevel);
				memcpy(m_remote_bda, param->connect.remote_bda, sizeof(m_remote_bda));
			}
			if (m_pServerCallbacks != nullptr) {
				m_pServerCallbacks->onConnect(this);
			}
			m_connectedCount++;   // Increment the number of connected devices count.
			break;
		} // ESP_GATTS_CONNECT_EVT


		// ESP_GATTS_CREATE_EVT
		// Called when a new service is registered as having been created.
		//
		// create:
		// * esp_gatt_status_t  status
		// * uint16_t           service_handle
		// * esp_gatt_srvc_id_t service_id
		//
		case ESP_GATTS_CREATE_EVT: {
			BLEService* pService = m_serviceMap.GetByUUID(param->create.service_id.id.uuid);
			m_serviceMap.SetByHandle(param->create.service_handle, pService);
			m_semaphoreCreateEvt.Give();
			break;
		} // ESP_GATTS_CREATE_EVT


		// ESP_GATTS_DISCONNECT_EVT
		//
		// disconnect
		// - uint16_t      conn_id
		// - esp_bd_addr_t remote_bda
		// - bool          is_connected
		//
		// If we receive a disconnect event then invoke the callback for disconnects (if one is present).
		// we also want to start advertising again.
		case ESP_GATTS_DISCONNECT_EVT: {
			m_connectedCount--;                          // Decrement the number of connected devices count.
			if (m_pServerCallbacks != nullptr) {         // If we have callbacks, call now.
				m_pServerCallbacks->onDisconnect(this);
			}
			StartAdvertising(); //- do this with some delay from the loop()
			break;
		} // ESP_GATTS_DISCONNECT_EVT


		// ESP_GATTS_READ_EVT - A request to read the value of a characteristic has arrived.
		//
		// read:
		// - uint16_t      conn_id
		// - uint32_t      trans_id
		// - esp_bd_addr_t bda
		// - uint16_t      handle
		// - uint16_t      offset
		// - bool          is_long
		// - bool          need_rsp
		//
		case ESP_GATTS_READ_EVT: {
			break;
		} // ESP_GATTS_READ_EVT


		// ESP_GATTS_REG_EVT
		// reg:
		// - esp_gatt_status_t status
		// - uint16_t app_id
		//
		case ESP_GATTS_REG_EVT: {
			m_gatts_if = gatts_if;
			m_semaphoreRegisterAppEvt.Give(); // Unlock the mutex waiting for the registration of the app.
			break;
		} // ESP_GATTS_REG_EVT


		// ESP_GATTS_WRITE_EVT - A request to write the value of a characteristic has arrived.
		//
		// write:
		// - uint16_t      conn_id
		// - uint16_t      trans_id
		// - esp_bd_addr_t bda
		// - uint16_t      handle
		// - uint16_t      offset
		// - bool          need_rsp
		// - bool          is_prep
		// - uint16_t      len
		// - uint8_t*      value
		//
		case ESP_GATTS_WRITE_EVT: {
			break;
		}

		default: {
			break;
		}
	}
	ESP_LOGD(LOG_TAG, "<< handleGATTServerEvent");
} // handleGATTServerEvent


/**
 * @brief Register the app.
 *
 * @return N/A
 */
void BLEServerGeneric::RegisterApp() {
	ESP_LOGD(LOG_TAG, ">> registerApp - %d", m_appId);
	m_semaphoreRegisterAppEvt.Take("registerApp"); // Take the mutex, will be released by ESP_GATTS_REG_EVT event.
	::esp_ble_gatts_app_register(m_appId);
	m_semaphoreRegisterAppEvt.Wait("registerApp");
	ESP_LOGD(LOG_TAG, "<< registerApp");
} // registerApp


/**
 * @brief Set the server callbacks.
 *
 * As a %BLE server operates, it will generate server level events such as a new client connecting or a previous client
 * disconnecting.  This function can be called to register a callback handler that will be invoked when these
 * events are detected.
 *
 * @param [in] pCallbacks The callbacks to be invoked.
 */
void BLEServerGeneric::SetCallbacks(BLEServerCallbacks* pCallbacks) {
	m_pServerCallbacks = pCallbacks;
} // setCallbacks


/**
 * @brief Start advertising.
 *
 * Start the server advertising its existence.  This is a convenience function and is equivalent to
 * retrieving the advertising object and invoking start upon it.
 */
void BLEServerGeneric::StartAdvertising() {
	ESP_LOGD(LOG_TAG, ">> startAdvertising");
	m_bleAdvertising.Start();
	ESP_LOGD(LOG_TAG, "<< startAdvertising");
} // startAdvertising

void BLEServerGeneric::SetEncryptionLevel(esp_ble_sec_act_t level) {
	m_securityLevel = level;
}

uint32_t BLEServerGeneric::GetPassKey() {
	return m_securityPassKey;
}

void BLEServerGeneric::SetSecurityCallbacks(BLESecurityCallbacks* callbacks) {
	m_securityCallbacks = callbacks;
}

void BLEServerCallbacks::onConnect(BLEServerGeneric* pServer) {
	ESP_LOGD("BLEServerCallbacks", ">> onConnect(): Default");
	ESP_LOGD("BLEServerCallbacks", "Device: %s", BLEDevice::toString().c_str());
	ESP_LOGD("BLEServerCallbacks", "<< onConnect()");
} // onConnect

void BLEServerCallbacks::onDisconnect(BLEServerGeneric* pServer) {
	ESP_LOGD("BLEServerCallbacks", ">> onDisconnect(): Default");
	ESP_LOGD("BLEServerCallbacks", "Device: %s", BLEDevice::toString().c_str());
	ESP_LOGD("BLEServerCallbacks", "<< onDisconnect()");
} // onDisconnect

#endif // CONFIG_BT_ENABLED
