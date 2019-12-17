/*
 *    BLESecurity.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLESECURITY_H_
#define DRIVERS_BLESECURITY_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <esp_gap_ble_api.h>

class BLESecurity {
	public:
		BLESecurity();
		virtual ~BLESecurity();
		void 	SetAuthenticationMode(esp_ble_auth_req_t auth_req);
		void 	SetCapability(esp_ble_io_cap_t iocap);
		void 	SetInitEncryptionKey(uint8_t init_key);
		void 	SetRespEncryptionKey(uint8_t resp_key);
		void 	SetKeySize(uint8_t key_size = 16);
		void 	SetStaticPIN(uint32_t pin);
		static char* esp_key_type_to_str(esp_ble_key_type_t key_type);

	private:
		esp_ble_auth_req_t m_authReq;
		esp_ble_io_cap_t m_iocap;
		uint8_t m_initKey;
		uint8_t m_respKey;
		uint8_t m_keySize;

}; // BLESecurity


/*
 * @brief Callbacks to handle GAP events related to authorization
 */
class BLESecurityCallbacks {
	public:
		virtual ~BLESecurityCallbacks() {};

		/**
		 * @brief Its request from peer device to input authentication pin code displayed on peer device.
		 * It requires that our device is capable to input 6-digits code by end user
		 * @return Return 6-digits integer value from input device
		 */
		virtual uint32_t OnPassKeyRequest() = 0;

		/**
		 * @brief Provide us 6-digits code to perform authentication.
		 * It requires that our device is capable to display this code to end user
	 	 * @param
	 	 */
		virtual void OnPassKeyNotify(uint32_t pass_key) = 0;

		/**
		 * @brief Here we can make decision if we want to let negotiate authorization with peer device or not
		 * return Return true if we accept this peer device request
		 */

		virtual bool OnSecurityRequest() = 0 ;
		/**
		 * Provide us information when authentication process is completed
		 */
		virtual void OnAuthenticationComplete(esp_ble_auth_cmpl_t) = 0;

		virtual bool OnConfirmPIN(uint32_t pin) = 0;
}; // BLESecurityCallbacks

#endif // CONFIG_BT_ENABLED
#endif // COMPONENTS_CPP_UTILS_BLESECURITY_H_
