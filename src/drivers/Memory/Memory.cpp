#include "Memory.h"
#include <stdlib.h>
#include "esp_log.h"

static string ArrayCountName = "_count";

NVS::NVS(string name, nvs_open_mode openMode) {
	m_name = name;
	nvs_open(name.c_str(), openMode, &m_handle);
} // NVS

NVS::NVS(const char name[], nvs_open_mode openMode) {
	m_name = name;
	nvs_open(name, openMode, &m_handle);
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

	if (nvs_err == ESP_OK && length > 0) {
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
 * @brief Retrieve a uint 32 bit value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The uint read from the %NVS storage.
 */
uint32_t NVS::GetUInt32Bit(string key) {
	uint32_t data;
	esp_err_t nvs_err = nvs_get_u32(m_handle, key.c_str(), &data);

	return (nvs_err == ESP_OK) ? data : +0;
} // get

/**
 * @brief Set the uint 32 bit value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetUInt32Bit(string key, uint32_t data) {
	nvs_set_u32(m_handle, key.c_str(), data);
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
void NVS::BlobArrayAdd(string ArrayName, void *Item, size_t DataLength) {
	uint8_t Count = ArrayCount(ArrayName);
	if (Count < +MAX_NVSARRAY_INDEX) {
		SetBlob(ArrayName + "_" + Converter::ToString(Count), Item, DataLength);
		ArrayCountSet(ArrayName, Count);
	}
}

/**
 * @brief Retriev data from the blob array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 * @param [out] result void * read from the NVS array by given index. NULL if none result.
 */
void * NVS::BlobArrayGet(string ArrayName, uint8_t Index) {
	 return GetBlob(ArrayName + "_" + Converter::ToString(Index));
}

/**
 * @brief Remove row from the blob array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 */
void NVS::BlobArrayRemove(string ArrayName, uint8_t Index) {
	uint8_t Count = ArrayCount(ArrayName);

	Erase(ArrayName + "_" + Converter::ToString(Index));

	for (uint8_t i = Index; i < (Count-1) ; i++) {
		void * tmp = GetBlob(ArrayName + "_" + Converter::ToString((uint8_t)(i+1)));
		SetBlob(ArrayName + "_" + Converter::ToString((uint8_t)i), tmp);
	}

	Erase(ArrayName + "_" + Converter::ToString((uint8_t)(Count-1)));
	ArrayCountSet(ArrayName, (Count != 0) ? (Count-1) : 0);
}

/**
 * @brief Replace data in the array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 * @param [in] Item Item to replace item in NVS.
 */
void NVS::BlobArrayReplace(string ArrayName, uint8_t Index, void *Item, size_t DataLength) {
	 return SetBlob(ArrayName + "_" + Converter::ToString(Index), Item, DataLength);
}

/**
 * @brief Add item in the string array .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Item The value to add in the array.
 * @param [out] Inserted item index. If Failed = 255
 */
uint8_t NVS::StringArrayAdd(string ArrayName, string Item) {
	uint8_t Count = ArrayCount(ArrayName);
	if (Count < +MAX_NVSARRAY_INDEX) {
		SetString(ArrayName + "_" + Converter::ToString(Count), Item);
		ArrayCountSet(ArrayName, Count+1);
		return Count;
	}

	return MAX_NVSARRAY_INDEX +1;
}

/**
 * @brief Retriev data from the string array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 * @param [out] result string read from the NVS array by given index. NULL if none result.
 */
string NVS::StringArrayGet(string ArrayName, uint8_t Index) {
	 return GetString(ArrayName + "_" + Converter::ToString(Index));
}


/**
 * @brief Remove row from the string array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 */
void NVS::StringArrayRemove(string ArrayName, uint8_t Index) {
	uint8_t Count = ArrayCount(ArrayName);

	Erase(ArrayName + "_" + Converter::ToString(Index));

	for (uint8_t i = Index; i < (Count-1) ; i++) {
		string tmp = GetString(ArrayName + "_" + Converter::ToString((uint8_t)(i+1)));
		SetString(ArrayName + "_" + Converter::ToString((uint8_t)i), tmp);
	}

	Erase(ArrayName + "_" + Converter::ToString((uint8_t)(Count-1)));
	ArrayCountSet(ArrayName, (Count != 0) ? (Count-1) : 0);
}

/**
 * @brief Replace data in the array by index .
 *
 * @param [in] ArrayName Array name.
 * @param [in] Index The index of the neccessary array element.
 * @param [in] Item Item to replace item in NVS.
 */
void NVS::StringArrayReplace(string ArrayName, uint8_t Index, string Item) {
	 return SetString(ArrayName + "_" + Converter::ToString(Index), Item);
}

/**
 * @brief Set Count item for specified array.
 *
 * @param [in] ArrayName Name of Array in that you need to set count.
 * @param [in] Count Aray items count value
 */
void NVS::ArrayCountSet(string ArrayName, uint8_t Count) {
	uint8_t ArrayCount = (Count >= +128) ? +0 : Count; 	// проверка на переполнение
	SetInt8Bit(ArrayName + ArrayCountName, ArrayCount);
}

/**
 * @brief Get array items count for specified array.
 *
 * @param [in] ArrayName Name of Array in that you need to set count.
 * @param [out] Array items count
 */
uint8_t NVS::ArrayCount(string ArrayName) {
	uint8_t ArrayCount = GetInt8Bit(ArrayName + ArrayCountName);
	return (ArrayCount >= +128) ? +0 : ArrayCount; // проверка на переполнение
}

/**
 * @brief Clean all data, associated with given array.
 *
 * @param [in] ArrayName Name of Array that you need to clean.
 */
void NVS::ArrayEraseAll(string ArrayName) {

	for (uint8_t i = 0; i < 128; i++)
		Erase(ArrayName + "_" + Converter::ToString(i));

	Erase(ArrayName + ArrayCountName);
}
