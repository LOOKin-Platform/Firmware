/*
*    NVSWrapper.h
*    Works with ESP32 NVS API
*
*/

#ifndef DRIVERS_NVS_H_
#define DRIVERS_NVS_H_

using namespace std;

#include <nvs.h>
#include <nvs_flash.h>
#include <string>

#include "Converter.h"

#define MAX_NVSARRAY_INDEX	128

class NVS {
	public:
		static void Init();
		static void Init(string Partition);

		static void Deinit(string Partition);

		NVS(string name, nvs_open_mode openMode = NVS_READWRITE);
		NVS(string Partition, string name, nvs_open_mode openMode = NVS_READWRITE);

		virtual 			~NVS();
		void 				Commit();

		bool 				IsKeyExists(string Key);

		void 				Erase();
		void 				Erase(string key);
		void 				EraseStartedWith(string Key);
		void 				EraseNamespace();
		static void			ClearAll();

		esp_err_t			GetLastError();

		vector<string>		FindAllStartedWith(string Key);

		string 				GetString(string key);
		bool 				SetString(string key, string data);

		uint8_t 			GetInt8Bit(string key);
		void 				SetInt8Bit(string key, uint8_t data);

		uint16_t 			GetUInt16Bit(string key);
		void 				SetUInt16Bit(string key, uint16_t data);

		uint32_t 			GetUInt32Bit(string key);
		void 				SetUInt32Bit(string key, uint32_t data);

		uint64_t 			GetUInt64Bit(string key);
		void 				SetUInt64Bit(string key, uint64_t data);


		pair<void *, size_t>GetBlob(string key);
		void 				SetBlob(string key, void *data, size_t datalen = 0);
		bool				CheckBlobExists(string Key);

		/* Methods to work with blob arrays in NVS */

		void 				BlobArrayAdd		(string ArrayName, void *Item, size_t datalen = 0);
		void * 				BlobArrayGet		(string ArrayName, uint8_t Index);
		void 				BlobArrayRemove		(string ArrayName, uint8_t Index);
		void 				BlobArrayReplace	(string ArrayName, uint8_t Index, void *Item, size_t datalen = 0);

		/* Methods to work with string arrays in NVS */

		uint8_t				StringArrayAdd		(string ArrayName, string Item);
		string 				StringArrayGet		(string ArrayName, uint8_t Index);
		void 				StringArrayRemove	(string ArrayName, uint8_t Index);
		esp_err_t			StringArrayReplace	(string ArrayName, uint8_t Index, string Item);

		/* General arrays methods */

		void 				ArrayEraseAll	(string ArrayName);
		uint8_t 			ArrayCount		(string ArrayName);

	private:
		string 				m_name;
		nvs_handle 			m_handle;
		esp_err_t			m_lasterror;

		static bool 		isInited;

		void 				ArrayCountSet(string ArrayName, uint8_t Count);
};

#endif
