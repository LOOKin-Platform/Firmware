/*
 *    BLEServiceMap.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <sstream>
#include <iomanip>
#include "BLEService.h"


/**
 * @brief Return the service by UUID.
 * @param [in] UUID The UUID to look up the service.
 * @return The characteristic.
 */
BLEService* BLEServiceMap::GetByUUID(const char* uuid) {
	return GetByUUID(BLEUUID(uuid));
}

/**
 * @brief Return the service by UUID.
 * @param [in] UUID The UUID to look up the service.
 * @return The characteristic.
 */
BLEService* BLEServiceMap::GetByUUID(BLEUUID uuid, uint8_t inst_id) {
	for (auto &myPair : m_uuidMap) {
		if (myPair.first->GetUUID().Equals(uuid)) {
			return myPair.first;
		}
	}
	//return m_uuidMap.at(uuid.ToString());
	return nullptr;
} // getByUUID


/**
 * @brief Return the service by handle.
 * @param [in] handle The handle to look up the service.
 * @return The service.
 */
BLEService* BLEServiceMap::GetByHandle(uint16_t handle) {
	return m_handleMap.at(handle);
} // getByHandle


/**
 * @brief Set the service by UUID.
 * @param [in] uuid The uuid of the service.
 * @param [in] characteristic The service to cache.
 * @return N/A.
 */
void BLEServiceMap::SetByUUID(BLEUUID uuid, BLEService* service) {
	m_uuidMap.insert(std::pair<BLEService*, std::string>(service, uuid.ToString()));
} // setByUUID


/**
 * @brief Set the service by handle.
 * @param [in] handle The handle of the service.
 * @param [in] service The service to cache.
 * @return N/A.
 */
void BLEServiceMap::SetByHandle(uint16_t handle, BLEService* service) {
	m_handleMap.insert(std::pair<uint16_t, BLEService*>(handle, service));
} // setByHandle


/**
 * @brief Return a string representation of the service map.
 * @return A string representation of the service map.
 */
std::string BLEServiceMap::ToString() {
	std::stringstream stringStream;
	stringStream << std::hex << std::setfill('0');
	for (auto &myPair: m_handleMap) {
		stringStream << "handle: 0x" << std::setw(2) << myPair.first << ", uuid: " + myPair.second->GetUUID().ToString() << "\n";
	}
	return stringStream.str();
} // toString

void BLEServiceMap::HandleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {
	// Invoke the handler for every Service we have.
	for (auto &myPair : m_uuidMap) {
		myPair.first->HandleGATTServerEvent(event, gatts_if, param);
	}
}

/**
 * @brief Get the first service in the map.
 * @return The first service in the map.
 */
BLEService* BLEServiceMap::GetFirst() {
	m_iterator = m_uuidMap.begin();
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLEService* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} // getFirst

/**
 * @brief Get the next service in the map.
 * @return The next service in the map.
 */
BLEService* BLEServiceMap::GetNext() {
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLEService* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} // getNext

/**
 * @brief Removes service from maps.
 * @return N/A.
 */
void BLEServiceMap::RemoveService(BLEService* service) {
	m_handleMap.erase(service->GetHandle());
	m_uuidMap.erase(service);
} // removeService

/**
 * @brief Returns the amount of registered services
 * @return amount of registered services
 */
int BLEServiceMap::GetRegisteredServiceCount(){
	return m_handleMap.size();
}

#endif /* CONFIG_BT_ENABLED */
