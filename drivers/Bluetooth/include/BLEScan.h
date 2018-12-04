/*
 *    BLEScan.cpp
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLESCAN_H_
#define DRIVERS_BLESCAN_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_gap_ble_api.h>

#include <vector>
#include "BLEAdvertisedDevice.h"
#include "BLEClientGeneric.h"
#include "FreeRTOSWrapper.h"

class BLEAdvertisedDevice;
class BLEAdvertisedDeviceCallbacks;
class BLEClientGeneric;
class BLEScan;

/**
 * @brief The result of having performed a scan.
 * When a scan completes, we have a set of found devices.  Each device is described
 * by a BLEAdvertisedDevice object.  The number of items in the set is given by
 * getCount().  We can retrieve a device by calling getDevice() passing in the
 * index (starting at 0) of the desired device.
 */
class BLEScanResults {
	public:
		void                Dump();
		int                 GetCount();
		BLEAdvertisedDevice GetDevice(uint32_t i);

private:
	friend BLEScan;
	std::vector<BLEAdvertisedDevice>
		m_vectorAdvertisedDevices;
};

/**
 * @brief Perform and manage %BLE scans.
 *
 * Scanning is associated with a %BLE client that is attempting to locate BLE servers.
 */
class BLEScan {
	public:
		void			SetActiveScan(bool active);
		void			SetAdvertisedDeviceCallbacks(BLEAdvertisedDeviceCallbacks* pAdvertisedDeviceCallbacks, bool wantDuplicates = false);
		void			SetInterval(uint16_t intervalMSecs);
		void			SetWindow(uint16_t windowMSecs);
		BLEScanResults	Start(uint32_t duration);
		void			Stop();

		uint32_t		ScanDuration = 0;

	private:
		BLEScan();		// One doesn't create a new instance instead one asks the BLEDevice for the singleton.
		friend class	BLEDevice;
		void			HandleGAPEvent(esp_gap_ble_cb_event_t  event, esp_ble_gap_cb_param_t* param);
		void			ParseAdvertisement(BLEClientGeneric* pRemoteDevice, uint8_t *payload);


		esp_ble_scan_params_t			m_scan_params;
		BLEAdvertisedDeviceCallbacks*	m_pAdvertisedDeviceCallbacks;
		bool							m_stopped;
		FreeRTOS::Semaphore				m_semaphoreScanEnd = FreeRTOS::Semaphore("ScanEnd");
		BLEScanResults					m_scanResults;
		bool							m_wantDuplicates;
}; // BLEScan

#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLESCAN_H_ */
