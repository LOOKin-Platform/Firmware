/*
*    BLEAddress.h
*    Bluetooth driver
*
*/

#ifndef DRIVERS_BLEADDRESS_H_
#define DRIVERS_BLEADDRESS_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <string>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <stdio.h>

#include <esp_gap_ble_api.h> // ESP32 BLE

using namespace std;

/**
 * @brief A %BLE device address.
 *
 * Every %BLE device has a unique address which can be used to identify it and form connections.
 */
class BLEAddress {
	public:
		BLEAddress		(esp_bd_addr_t address);
		BLEAddress		(string stringAddress);
		bool           	equals(BLEAddress otherAddress);
		esp_bd_addr_t*	getNative();
		string   		toString();

	private:
		esp_bd_addr_t m_address;
};

#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLEADDRESS_H_ */
