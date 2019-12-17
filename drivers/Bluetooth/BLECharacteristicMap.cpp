/*
 *    BLECharacteristicMap.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <sstream>
#include <iomanip>
#include "BLEService.h"


/**
 * @brief Return the characteristic by handle.
 * @param [in] handle The handle to look up the characteristic.
 * @return The characteristic.
 */
BLECharacteristic* BLECharacteristicMap::GetByHandle(uint16_t handle) {
	return m_handleMap.at(handle);
} // getByHandle


/**
 * @brief Return the characteristic by UUID.
 * @param [in] UUID The UUID to look up the characteristic.
 * @return The characteristic.
 */
BLECharacteristic* BLECharacteristicMap::GetByUUID(const char* uuid) {
    return GetByUUID(BLEUUID(uuid));
}


/**
 * @brief Return the characteristic by UUID.
 * @param [in] UUID The UUID to look up the characteristic.
 * @return The characteristic.
 */
BLECharacteristic* BLECharacteristicMap::GetByUUID(BLEUUID uuid) {
	for (auto &myPair : m_uuidMap) {
		if (myPair.first->GetUUID().Equals(uuid)) {
			return myPair.first;
		}
	}
	//return m_uuidMap.at(uuid.ToString());
	return nullptr;
} // getByUUID


/**
 * @brief Get the first characteristic in the map.
 * @return The first characteristic in the map.
 */
BLECharacteristic* BLECharacteristicMap::GetFirst() {
	m_iterator = m_uuidMap.begin();
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLECharacteristic* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} // getFirst


/**
 * @brief Get the next characteristic in the map.
 * @return The next characteristic in the map.
 */
BLECharacteristic* BLECharacteristicMap::GetNext() {
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLECharacteristic* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} // getNext


/**
 * @brief Pass the GATT server event onwards to each of the characteristics found in the mapping
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 */
void BLECharacteristicMap::HandleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	// Invoke the handler for every Service we have.
	for (auto& myPair : m_uuidMap) {
		myPair.first->HandleGATTServerEvent(event, gatts_if, param);
	}
} // handleGATTServerEvent


/**
 * @brief Set the characteristic by handle.
 * @param [in] handle The handle of the characteristic.
 * @param [in] characteristic The characteristic to cache.
 * @return N/A.
 */
void BLECharacteristicMap::SetByHandle(uint16_t handle, BLECharacteristic* characteristic) {
	m_handleMap.insert(std::pair<uint16_t, BLECharacteristic*>(handle, characteristic));
} // setByHandle


/**
 * @brief Set the characteristic by UUID.
 * @param [in] uuid The uuid of the characteristic.
 * @param [in] characteristic The characteristic to cache.
 * @return N/A.
 */
void BLECharacteristicMap::SetByUUID(BLECharacteristic* pCharacteristic, BLEUUID uuid) {
	m_uuidMap.insert(pair<BLECharacteristic*, std::string>(pCharacteristic, uuid.ToString()));
} // setByUUID


/**
 * @brief Return a string representation of the characteristic map.
 * @return A string representation of the characteristic map.
 */
std::string BLECharacteristicMap::ToString() {
	std::stringstream stringStream;
	stringStream << std::hex << std::setfill('0');
	int count = 0;
	for (auto &myPair: m_uuidMap) {
		if (count > 0) {
			stringStream << "\n";
		}
		count++;
		stringStream << "handle: 0x" << std::setw(2) << myPair.first->GetHandle() << ", uuid: " + myPair.first->GetUUID().ToString();
	}
	return stringStream.str();
} // toString

#endif
