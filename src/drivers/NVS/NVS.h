#ifndef DRIVERS_NVS_H_
#define DRIVERS_NVS_H_

using namespace std;

#include <nvs.h>
#include <string>

#include "Tools.h"

class NVS {
public:
	NVS(string name, nvs_open_mode openMode = NVS_READWRITE);
	virtual ~NVS();
	void 		Commit();

	void 		Erase();
	void 		Erase(string key);

	string 	GetString(string key);
	void 		SetString(string key, string data);

	uint8_t GetInt8Bit(string key);
	void 		SetInt8Bit(string key, uint8_t data);

	void * 	GetBlob(string key);
	void 		SetBlob(string key, void *data, size_t datalen = 0);

	/* Операции с blob-массивами в NVS */

	void 		ArrayAdd			(string ArrayName, void *Item, size_t datalen = 0);
	void * 	ArrayGet			(string ArrayName, uint8_t Index);
	void 		ArrayRemove		(string ArrayName, uint8_t Index);
	void 		ArrayReplace	(string ArrayName, uint8_t Index, void *Item, size_t datalen = 0);

	void 		ArrayEraseAll	(string ArrayName);
	uint8_t ArrayCount		(string ArrayName);
private:
	string 			m_name;
	nvs_handle 	m_handle;

	void ArrayCountSet(string ArrayName, uint8_t Count);
};

#endif
