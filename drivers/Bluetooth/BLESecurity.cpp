/*
 *    BLESecurity.cpp
 *    Bluetooth driver
 *
 */

#include <BLESecurity.h>

BLESecurity::BLESecurity() {
}

BLESecurity::~BLESecurity() {
}
/*
 * @brief Set requested authentication mode
 */
void BLESecurity::setAuthenticationMode(esp_ble_auth_req_t auth_req) {
	m_authReq = auth_req;
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &m_authReq, sizeof(uint8_t));		  // <--- setup requested authentication mode
}
/*
 * @brief 	Set our device IO capability to let end user perform authorization
 * 			either by displaying or entering generated 6-digits pin code
 */
void BLESecurity::setCapability(esp_ble_io_cap_t iocap) {
	m_iocap = iocap;
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
}
/*
 * @brief Init encryption key by server
 * @param key_size is value between 7 and 16
 */
void BLESecurity::setInitEncryptionKey(uint8_t init_key, uint8_t key_size) {
	m_initKey = init_key;
	m_keySize = key_size;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &m_initKey, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));		// <--- setup encryption key max size
}

/*
 * @brief Init encryption key by client
 * @param key_size is value between 7 and 16
 */
void BLESecurity::setRespEncryptionKey(uint8_t resp_key, uint8_t key_size) {
	m_respKey = resp_key;
	m_keySize = key_size;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &m_respKey, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));		// <--- setup encryption key max size
}
/*
 * @brief Debug function to display what keys are exchanged by peers
 */
char* BLESecurity::esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = (char*)"ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = (char*)"ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = (char*)"ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = (char*)"ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = (char*)"ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = (char*)"ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = (char*)"ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = (char*)"ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = (char*)"ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = (char*)"INVALID BLE KEY TYPE";
        break;

    }
     return key_str;
}
