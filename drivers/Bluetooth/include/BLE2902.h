/*
*    BLE2902.h
*    Bluetooth driver
*
*/

#ifndef DRIVERS_BLE2902_H_
#define DRIVERS_BLE2902_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLEDescriptor.h"

/**
 * @brief Descriptor for Client Characteristic Configuration.
 *
 * This is a convenience descriptor for the Client Characteristic Configuration which has a UUID of 0x2902.
 *
 */

class BLE2902: public BLEDescriptor {
	public:
		BLE2902();
		bool GetNotifications();
		bool GetIndications();
		void SetNotifications(bool flag);
		void SetIndications(bool flag);

}; // BLE2902

#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLE2902_H_ */
