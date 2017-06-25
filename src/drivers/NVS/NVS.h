#ifndef DRIVERS_NVS_H_
#define DRIVERS_NVS_H_
#include <nvs.h>
#include <string>

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

private:
	std::string m_name;
	nvs_handle m_handle;
};

#endif
