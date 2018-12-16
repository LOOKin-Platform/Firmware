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

		NVS(string name, nvs_open_mode openMode = NVS_READWRITE);
		NVS(const char[], nvs_open_mode openMode = NVS_READWRITE);

		virtual 	~NVS();
		void 		Commit();

		void 		Erase();
		void 		Erase(string key);

		string 		GetString(string key);
		void 		SetString(string key, string data);

		uint8_t 	GetInt8Bit(string key);
		void 		SetInt8Bit(string key, uint8_t data);

		uint32_t 	GetUInt32Bit(string key);
		void 		SetUInt32Bit(string key, uint32_t data);

		void * 		GetBlob(string key);
		void 		SetBlob(string key, void *data, size_t datalen = 0);

		/* Methods to work with blob arrays in NVS */

		void 		BlobArrayAdd		(string ArrayName, void *Item, size_t datalen = 0);
		void * 		BlobArrayGet		(string ArrayName, uint8_t Index);
		void 		BlobArrayRemove		(string ArrayName, uint8_t Index);
		void 		BlobArrayReplace	(string ArrayName, uint8_t Index, void *Item, size_t datalen = 0);

		/* Methods to work with string arrays in NVS */

		uint8_t		StringArrayAdd		(string ArrayName, string Item);
		string 		StringArrayGet		(string ArrayName, uint8_t Index);
		void 		StringArrayRemove	(string ArrayName, uint8_t Index);
		void 		StringArrayReplace	(string ArrayName, uint8_t Index, string Item);

		/* General arrays methods */

		void 		ArrayEraseAll	(string ArrayName);
		uint8_t 	ArrayCount		(string ArrayName);

	private:
		string 		m_name;
		nvs_handle 	m_handle;

		static bool isInited;

		void ArrayCountSet(string ArrayName, uint8_t Count);
};

#endif
