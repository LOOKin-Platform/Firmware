/*
 *    BLEAdvertisedDevice.h
 *    Bluetooth driver
 *
 *    During the scanning procedure, we will be finding advertised BLE devices.  This class
 *    models a found device.
 *
 *    See also:
 *    https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 *
 */

#ifndef DRIVERS_BLEADVERTISEDDEVICE_H_
#define DRIVERS_BLEADVERTISEDDEVICE_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <map>
#include <sstream>

#include "BLEAddress.h"
#include "BLEScan.h"
#include "BLEUUID.h"

#include <esp_gattc_api.h>
#include <esp_log.h>

class BLEScan;

/**
 * @brief A representation of a %BLE advertised device found by a scan.
 *
 * When we perform a %BLE scan, the result will be a set of devices that are advertising.  This
 * class provides a model of a detected device.
 */
class BLEAdvertisedDevice {
	public:
		BLEAdvertisedDevice();

		BLEAddress			GetAddress();
		uint16_t			GetAppearance();
		string				GetManufacturerData();
		string				GetName();
		int					GetRSSI();
		BLEScan*			GetScan();
		string				GetServiceData();
		BLEUUID				GetServiceDataUUID();
		BLEUUID				GetServiceUUID();
		int8_t				GetTXPower();
		uint8_t* 			GetPayload();
		size_t				GetPayloadLength();
		esp_ble_addr_type_t	GetAddressType();
		void 				SetAddressType(esp_ble_addr_type_t type);


		bool				IsAdvertisingService(BLEUUID uuid);
		bool				HaveAppearance();
		bool				HaveManufacturerData();
		bool				HaveName();
		bool				HaveRSSI();
		bool				HaveServiceData();
		bool				HaveServiceUUID();
		bool				HaveTXPower();

		string 				ToString();

	private:
		friend class BLEScan;

		void				ParseAdvertisement(uint8_t* payload, size_t total_len=62);
		void				SetAddress(BLEAddress address);
		void				SetAdFlag(uint8_t adFlag);
		void				SetAdvertizementResult(uint8_t* payload);
		void				SetAppearance(uint16_t appearance);
		void				SetManufacturerData(std::string manufacturerData);
		void				SetName(std::string name);
		void				SetRSSI(int rssi);
		void				SetScan(BLEScan* pScan);
		void				SetServiceData(std::string data);
		void				SetServiceDataUUID(BLEUUID uuid);
		void				SetServiceUUID(const char* serviceUUID);
		void				SetServiceUUID(BLEUUID serviceUUID);
		void				SetTXPower(int8_t txPower);


		bool				m_haveAppearance;
		bool				m_haveManufacturerData;
		bool				m_haveName;
		bool				m_haveRSSI;
		bool				m_haveServiceData;
		bool				m_haveServiceUUID;
		bool				m_haveTXPower;


		BLEAddress			m_address = BLEAddress((uint8_t*)"\0\0\0\0\0\0");
		uint8_t				m_adFlag;
		uint16_t			m_appearance;
		int					m_deviceType;
		string				m_manufacturerData;
		string				m_name;
		BLEScan*			m_pScan;
		int					m_rssi;
		vector<BLEUUID>		m_serviceUUIDs;
		int8_t				m_txPower;
		string				m_serviceData;
		BLEUUID				m_serviceDataUUID;

		uint8_t*			m_payload = nullptr;
		size_t				m_payloadLength = 0;
		esp_ble_addr_type_t	m_addressType;
};

/**
 * @brief A callback handler for callbacks associated device scanning.
 *
 * When we are performing a scan as a %BLE client, we may wish to know when a new device that is advertising
 * has been found.  This class can be sub-classed and registered such that when a scan is performed and
 * a new advertised device has been found, we will be called back to be notified.
 */
class BLEAdvertisedDeviceCallbacks {
	public:
		virtual ~BLEAdvertisedDeviceCallbacks() {}
		/**
		 * @brief Called when a new scan result is detected.
		 *
		 * As we are scanning, we will find new devices.  When found, this call back is invoked with a reference to the
		 * device that was found.  During any individual scan, a device will only be detected one time.
		 */
		virtual void OnResult(BLEAdvertisedDevice advertisedDevice);
		virtual void OnResult(BLEAdvertisedDevice* advertisedDevice);
};

#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLEADVERTISEDDEVICE_H_ */
