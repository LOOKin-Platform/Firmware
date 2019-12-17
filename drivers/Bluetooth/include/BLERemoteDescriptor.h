/*
 *    BLERemoteDescriptor.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEREMOTEDESCRIPTOR_H_
#define DRIVERS_BLEREMOTEDESCRIPTOR_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <string>

#include <esp_gattc_api.h>

#include "BLERemoteCharacteristic.h"
#include "BLEUUID.h"
#include "FreeRTOSWrapper.h"

class BLERemoteCharacteristic;
/**
 * @brief A model of remote %BLE descriptor.
 */
class BLERemoteDescriptor {
public:
	uint16_t    GetHandle();
	BLERemoteCharacteristic* GetRemoteCharacteristic();
	BLEUUID     GetUUID();
	std::string ReadValue(void);
	uint8_t     ReadUInt8(void);
	uint16_t    ReadUInt16(void);
	uint32_t    ReadUInt32(void);
	string 		ToString(void);
	void        WriteValue(uint8_t* data, size_t length, bool response = false);
	void        WriteValue(std::string newValue, bool response = false);
	void        WriteValue(uint8_t newValue, bool response = false);


private:
	friend class BLERemoteCharacteristic;
	BLERemoteDescriptor(
		uint16_t                 handle,
		BLEUUID                  uuid,
		BLERemoteCharacteristic* pRemoteCharacteristic
	);
	uint16_t                 m_handle;                  // Server handle of this descriptor.
	BLEUUID                  m_uuid;                    // UUID of this descriptor.
	std::string              m_value;                   // Last received value of the descriptor.
	BLERemoteCharacteristic* m_pRemoteCharacteristic;   // Reference to the Remote characteristic of which this descriptor is associated.
	FreeRTOS::Semaphore      m_semaphoreReadDescrEvt      = FreeRTOS::Semaphore("ReadDescrEvt");


};
#endif /* CONFIG_BT_ENABLED */
#endif /* COMPONENTS_CPP_UTILS_BLEREMOTEDESCRIPTOR_H_ */
