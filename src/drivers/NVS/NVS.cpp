using namespace std;

#include <stdlib.h>
#include "NVS.h"

NVS::NVS(std::string name, nvs_open_mode openMode) {
	m_name = name;
	nvs_open(name.c_str(), openMode, &m_handle);
} // NVS


NVS::~NVS() {
	nvs_close(m_handle);
} // ~NVS


void NVS::Commit() {
	nvs_commit(m_handle);
} // commit


void NVS::Erase() {
	nvs_erase_all(m_handle);
} // erase


/**
 * @brief Erase a specific key in the namespace.
 *
 * @param [in] key The key to erase from the namespace.
 */
void NVS::Erase(std::string key) {
	nvs_erase_key(m_handle, key.c_str());
} // erase


/**
 * @brief Retrieve a string value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The string read from the %NVS storage.
 */
string NVS::GetString(std::string key) {
	size_t length;
	nvs_get_str(m_handle, key.c_str(), NULL, &length);
	char *data = (char *)malloc(length);
	nvs_get_str(m_handle, key.c_str(), data, &length);
	string Result = string(data);
	free(data);

	return Result;
} // get


/**
 * @brief Set the string value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetString(std::string key, std::string data) {
	nvs_set_str(m_handle, key.c_str(), data.c_str());
} // set

/**
 * @brief Retrieve a uint 8 bit value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The uint read from the %NVS storage.
 */
uint8_t NVS::GetInt8Bit(string key) {
	uint8_t data;
	nvs_get_u8(m_handle, key.c_str(), &data);
	return data;
} // get

/**
 * @brief Set the uint 8 bit value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetInt8Bit(std::string key, uint8_t data) {
	nvs_set_u8(m_handle, key.c_str(), data);
} // set
