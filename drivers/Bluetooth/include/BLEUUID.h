/*
 *    BLEUUID.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEUUID_H_
#define DRIVERS_BLEUUID_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_gatt_defs.h>
#include <string>

using namespace std;

/**
 * @brief A model of a %BLE UUID.
 */
class BLEUUID {
	public:
		BLEUUID		(string uuid);
		BLEUUID		(uint16_t uuid);
		BLEUUID		(uint32_t uuid);
		BLEUUID		(esp_bt_uuid_t uuid);
		BLEUUID		(uint8_t* pData, size_t size, bool msbFirst);
		BLEUUID		(esp_gatt_id_t gattId);
		BLEUUID		();
		uint8_t BitSize	();   // Get the number of bits in this uuid.
		bool Equals	(BLEUUID uuid);
		esp_bt_uuid_t* GetNative();
		BLEUUID To128();
		string ToString();
		static BLEUUID FromString(string uuid); // Create a BLEUUID from a string

	private:
		esp_bt_uuid_t m_uuid; 	// The underlying UUID structure that this class wraps.
		bool m_valueSet;   		// Is there a value set for this instance.
};
// BLEUUID
#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLEUUID_H_ */
