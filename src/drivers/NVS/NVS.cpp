#include "NVS.h"
#include <stdlib.h>
#include "esp_log.h"

static string ArrayCountName = "_count";

NVS::NVS(string name, nvs_open_mode openMode) {
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
void NVS::Erase(string key) {
	nvs_erase_key(m_handle, key.c_str());
} // erase

/**
 * @brief Retrieve a string value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The string read from the %NVS storage.
 */
string NVS::GetString(string key) {
	size_t length;
	string Result = "";

	esp_err_t nvs_err = nvs_get_str(m_handle, key.c_str(), NULL, &length);

	if (nvs_err == ESP_OK && length > 0)
	{
		char *data = (char *)malloc(length);
		nvs_get_str(m_handle, key.c_str(), data, &length);
		Result = string(data);

		free(data);
	}

	return Result;
} // get

/**
 * @brief Set the string value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetString(string key, string data) {
	string ttt = data;
	nvs_set_str(m_handle, key.c_str(), ttt.c_str());
} // set

/**
 * @brief Retrieve a uint 8 bit value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The uint read from the %NVS storage.
 */
uint8_t NVS::GetInt8Bit(string key) {
	uint8_t data;
	esp_err_t nvs_err = nvs_get_u8(m_handle, key.c_str(), &data);

	return (nvs_err == ESP_OK) ? data : +0;
} // get

/**
 * @brief Set the uint 8 bit value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetInt8Bit(string key, uint8_t data) {
	nvs_set_u8(m_handle, key.c_str(), data);
} // set


/**
 * @brief Retrieve a blob value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result void * read from the %NVS storage.
 */
void * NVS::GetBlob(string key) {
	size_t Length = 0;  // value will default to 0, if not set yet in NVS

	esp_err_t nvs_err = nvs_get_blob(m_handle, key.c_str(), NULL, &Length);

	if (nvs_err == ESP_OK && Length > 0)
	{
		void *Value = malloc(Length + sizeof(uint32_t));
		nvs_get_blob(m_handle, key.c_str(), Value, &Length);

		return Value;
	}

	return NULL;
} // get


/**
 * @brief Set the blob value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetBlob(string key, void *Data, size_t datalen) {

	size_t Length = (datalen == 0) ? sizeof(Data) : datalen;
	nvs_set_blob(m_handle, key.c_str(), Data, Length);
} // set

/**
 * @brief Add item in the Blob array .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Item The value to add in the array.
 */
void NVS::ArrayAdd(string ArrayName, void *Item, size_t DataLength) {
	uint8_t Count = ArrayCount(ArrayName);
	if (Count < +128) {
		SetBlob(ArrayName + "_" + Tools::ToString(Count), Item, DataLength);
		ArrayCountSet(ArrayName, Count+1);
	}
}

/**
 * @brief Retriev data from the blob array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 * @param [out] result void * read from the NVS array by given index. NULL if none result.
 */
void * NVS::ArrayGet(string ArrayName, uint8_t Index) {
	 return GetBlob(ArrayName + "_" + Tools::ToString(Index));
}

/**
 * @brief Remove row from the blob array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 */
void NVS::ArrayRemove(string ArrayName, uint8_t Index) {
	uint8_t ArrayCount = GetInt8Bit(ArrayName);

	Erase(ArrayName + "_" + Tools::ToString(ArrayCount));

	for (uint8_t i = Index; i < (ArrayCount-1) ; i++) {
		void * tmp = GetBlob(ArrayName + "_" + Tools::ToString(i+1));
		SetBlob(ArrayName + "_" + Tools::ToString(i), tmp);
	}

	Erase(ArrayName + "_" + Tools::ToString(ArrayCount));
	ArrayCountSet(ArrayName, (ArrayCount-1));
}

/**
 * @brief Replace data in the array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 * @param [in] Item Item to replace item in NVS.
 */
void NVS::ArrayReplace(string ArrayName, uint8_t Index, void *Item, size_t DataLength) {
	 return SetBlob(ArrayName + "_" + Tools::ToString(Index), Item, DataLength);
}

void NVS::ArrayCountSet(string ArrayName, uint8_t Count) {
	 SetInt8Bit(ArrayName + ArrayCountName, Count);
}

uint8_t NVS::ArrayCount(string ArrayName) {
	return GetInt8Bit(ArrayName + ArrayCountName);
}

void NVS::ArrayEraseAll(string ArrayName) {

	for (uint8_t i = 0; i < 128; i++)
		Erase(ArrayName + "_" + Tools::ToString(i));

	Erase(ArrayName + ArrayCountName);
}
