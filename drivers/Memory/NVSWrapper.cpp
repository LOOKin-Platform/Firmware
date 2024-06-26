/*
*    NVSWrapper.cpp
*    Works with ESP32 NVS API
*
*/

#include "NVSWrapper.h"
#include <stdlib.h>
#include "esp_log.h"

static string ArrayCountName = "_count";

bool NVS::isInited = false;

void NVS::Init() {
	if (isInited) return;

    isInited = true;

    esp_err_t err = ::nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err == ESP_OK) 	{	ESP_LOGI("Default NVS", "NVS flash init success");				}
    else 				{	ESP_LOGE("Default NVS", "Error while NVS flash init, %d", err);	}
}

void NVS::Init(string Partition) {
    esp_err_t err = ::nvs_flash_init_partition(Partition.c_str());
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase_partition(Partition.c_str()));
        err = nvs_flash_init_partition(Partition.c_str());
    }

    if (err == ESP_OK) 	{	ESP_LOGI("Partition NVS", "NVS flash  partition init success");				}
    else 				{	ESP_LOGE("Partition NVS", "Error while NVS partition flash init, %d", err);	}
}

void NVS::Deinit(string Partition) {
	::nvs_flash_deinit_partition(Partition.c_str());
}

NVS::NVS(string name, nvs_open_mode openMode) {
	m_name = name;
	nvs_open(name.c_str(), openMode, &m_handle);
} // NVS


NVS::NVS(string Partition, string name, nvs_open_mode openMode) {
	m_name = name;
	m_lasterror = nvs_open_from_partition(Partition.c_str(), name.c_str(), openMode, &m_handle);
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

void NVS::EraseStartedWith(string Key) {
	nvs_iterator_t it = nullptr;
	esp_err_t res = ::nvs_entry_find(NVS_DEFAULT_PART_NAME, m_name.c_str(), NVS_TYPE_ANY, &it);

	while (res == ESP_OK) 
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		res = nvs_entry_next(&it);

		string ItemKey(info.key);
		string ItemKeyForComprasion  = Converter::ToLower(ItemKey);

		if (Converter::StartsWith(ItemKeyForComprasion, Converter::ToLower(Key)))
			Erase(ItemKey);
	}

	nvs_release_iterator(it);
}

bool NVS::IsKeyExists(string Key) {
	nvs_iterator_t it = nullptr;

	esp_err_t res = ::nvs_entry_find(NVS_DEFAULT_PART_NAME, m_name.c_str(), NVS_TYPE_ANY, &it);
	
	while (res == ESP_OK)
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		res = nvs_entry_next(&it);

		string ItemKey(info.key);

		if (Converter::ToLower(ItemKey) == Converter::ToLower(Key)) {
			nvs_release_iterator(it);
			return true;
		}
	}

	nvs_release_iterator(it);

	return false;
}

void NVS::EraseNamespace() {
	nvs_iterator_t it = nullptr;
	esp_err_t res = ::nvs_entry_find(NVS_DEFAULT_PART_NAME, m_name.c_str(), NVS_TYPE_ANY, &it);

	while (res == ESP_OK) 
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		res = nvs_entry_next(&it);

		Erase(info.key);
	};

	nvs_release_iterator(it);
}

void NVS::ClearAll() {
	nvs_flash_erase();

	isInited = false;
} // erase all items in NVS


esp_err_t NVS::GetLastError() {
	return m_lasterror;
}


/**
 * @brief Find all keys started by Key.
 *
 * @param [in] Key The start key to read from the namespace.
 */

vector<string> NVS::FindAllStartedWith(string Key) {
	vector<string> Result = vector<string>();

	nvs_iterator_t it = nullptr;
	esp_err_t res = ::nvs_entry_find(NVS_DEFAULT_PART_NAME, m_name.c_str(), NVS_TYPE_ANY, &it);

	while (res == ESP_OK) {
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		res = nvs_entry_next(&it);

		string ItemKey(info.key);
		string ItemKeyForComprasion  = Converter::ToLower(ItemKey);

		if (Converter::StartsWith(ItemKeyForComprasion, Converter::ToLower(Key)))
			Result.push_back(ItemKey);
	}

	nvs_release_iterator(it);

	return Result;
}


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
bool NVS::SetString(string key, string data) {
	esp_err_t Result = nvs_set_str(m_handle, key.c_str(), data.c_str());

	if (Result != ESP_OK)
		ESP_LOGE("NVSWrapper", "%s", ::esp_err_to_name(Result));

	return (Result == ESP_OK);
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
 * @brief Retrieve a int 16 bit value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The uint read from the %NVS storage.
 */
int16_t NVS::GetInt16Bit(string key) {
	int16_t data;
	esp_err_t nvs_err = nvs_get_i16(m_handle, key.c_str(), &data);

	return (nvs_err == ESP_OK) ? data : +0;
} // get

/**
 * @brief Set the int 16 bit value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetInt16Bit(string key, int16_t data) {
	nvs_set_i16(m_handle, key.c_str(), data);
} // set

/**
 * @brief Retrieve a uint 16 bit value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The uint read from the %NVS storage.
 */
uint16_t NVS::GetUInt16Bit(string key) {
	uint16_t data;
	esp_err_t nvs_err = nvs_get_u16(m_handle, key.c_str(), &data);

	return (nvs_err == ESP_OK) ? data : +0;
} // get

/**
 * @brief Set the uint 16 bit value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetUInt16Bit(string key, uint16_t data) {
	nvs_set_u16(m_handle, key.c_str(), data);
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
 * @brief Retrieve a uint 32 bit value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result The uint read from the %NVS storage.
 */
uint64_t NVS::GetUInt64Bit(string key) {
	uint64_t data;
	esp_err_t nvs_err = nvs_get_u64(m_handle, key.c_str(), &data);

	return (nvs_err == ESP_OK) ? data : +0;
} // get

/**
 * @brief Set the uint 32 bit value by key.
 *
 * @param [in] key The key to set from the namespace.
 * @param [in] data The value to set for the key.
 */
void NVS::SetUInt64Bit(string key, uint64_t data) {
	nvs_set_u64(m_handle, key.c_str(), data);
} // set

/**
 * @brief Retrieve a blob value by key.
 *
 * @param [in] key The key to read from the namespace.
 * @param [out] result void * read from the %NVS storage.
 */
pair<void *, size_t> NVS::GetBlob(string key) {
	size_t Length = 0;  // value will default to 0, if not set yet in NVS

	esp_err_t nvs_err = nvs_get_blob(m_handle, key.c_str(), NULL, &Length);

	if (nvs_err == ESP_OK && Length > 0)
	{
		void *Value = malloc(Length + sizeof(uint32_t));
		nvs_get_blob(m_handle, key.c_str(), Value, &Length);

		return make_pair(Value, Length);
	}

	return make_pair<void *, size_t>(NULL, 0);
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
 * @brief Check if blob exists.
 *
 * @param [in] key The key to set from the namespace.
 */

bool NVS::CheckBlobExists(string Key) {
	size_t Length = 0;  // value will default to 0, if not set yet in NVS

	esp_err_t nvs_err = nvs_get_blob(m_handle, Key.c_str(), NULL, &Length);

	return (nvs_err == ESP_OK && Length > 0);
}


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
	 return GetBlob(ArrayName + "_" + Converter::ToString(Index)).first;
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
		void * tmp = GetBlob(ArrayName + "_" + Converter::ToString((uint8_t)(i+1))).first;
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

	return MAX_NVSARRAY_INDEX+1;
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
esp_err_t NVS::StringArrayReplace(string ArrayName, uint8_t Index, string Item) {
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
