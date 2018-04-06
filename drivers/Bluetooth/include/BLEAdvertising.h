/*
 *    BLEAdvertising.h
 *    Bluetooth driver
 *
 * 	This class encapsulates advertising a BLE Server.
 *
 * 	The ESP-IDF provides a framework for BLE advertising.  It has determined that there are a common set
 * 	of properties that are advertised and has built a data structure that can be populated by the programmer.
 * 	This means that the programmer doesn't have to "mess with" the low level construction of a low level
 * 	BLE advertising frame.  Many of the fields are determined for us while others we can set before starting
 * 	to advertise.
 *
 *	Should we wish to construct our own payload, we can use the BLEAdvertisementData class and call the setters
 * 	upon it.  Once it is populated, we can then associate it with the advertising and what ever the programmer
 * 	set in the data will be advertised.
 *
 */
#ifndef DRIVERS_BLEADVERTISING_H_
#define DRIVERS_BLEADVERTISING_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <vector>

#include "BLEUUID.h"
#include "GeneralUtils.h"

#include <esp_log.h>
#include <esp_err.h>
#include <esp_gap_ble_api.h>

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

/**
 * @brief Representation of a beacon.
 * See:
 * * https://en.wikipedia.org/wiki/IBeacon
 */
class BLEBeacon {
private:
	struct {
		uint16_t manufacturerId;
		uint8_t  subType;
		uint8_t  subTypeLength;
		uint8_t  proximityUUID[16];
		uint16_t major;
		uint16_t minor;
		int8_t  signalPower;
	} __attribute__((packed))m_beaconData;
public:
	BLEBeacon();
	void setManufacturerId(uint16_t manufacturerId);
	//void setSubType(uint8_t subType);
	void setProximityUUID(BLEUUID uuid);
	void setMajor(uint16_t major);
	void setMinor(uint16_t minor);
	void setSignalPower(int8_t signalPower);
	std::string getData();
}; // BLEBeacon


/**
 * @brief Advertisement data set by the programmer to be published by the %BLE server.
 */
class BLEAdvertisementData {
	// Only a subset of the possible BLE architected advertisement fields are currently exposed.  Others will
	// be exposed on demand/request or as time permits.
	//
public:
	void setAppearance(uint16_t appearance);
	void setCompleteServices(BLEUUID uuid);
	void setFlags(uint8_t);
	void setManufacturerData(std::string data);
	void setName(std::string name);
	void setPartialServices(BLEUUID uuid);
	void setShortName(std::string name);

private:
	friend class BLEAdvertising;
	std::string m_payload;   // The payload of the advertisement.

	void        addData(std::string data);  // Add data to the payload.
	std::string getPayload();               // Retrieve the current advert payload.
};   // BLEAdvertisementData


/**
 * @brief Perform and manage %BLE advertising.
 *
 * A %BLE server will want to perform advertising in order to make itself known to %BLE clients.
 */
class BLEAdvertising {
public:
	BLEAdvertising();
	void addServiceUUID(BLEUUID serviceUUID);
	void addServiceUUID(const char* serviceUUID);
	void start();
	void stop();
	void setAppearance(uint16_t appearance);
	void setAdvertisementData(BLEAdvertisementData& advertisementData);
	void setScanFilter(bool scanRequertWhitelistOnly, bool connectWhitelistOnly);
	void setScanResponseData(BLEAdvertisementData& advertisementData);

private:
	esp_ble_adv_data_t   m_advData;
	esp_ble_adv_params_t m_advParams;
	std::vector<BLEUUID> m_serviceUUIDs;
	bool                 m_customAdvData;  // Are we using custom advertising data?
	bool                 m_customScanResponseData;  // Are we using custom scan response data?
};
#endif /* CONFIG_BT_ENABLED */
#endif /* DRIVERS_BLEADVERTISING_H_ */
