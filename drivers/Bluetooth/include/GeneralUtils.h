/*
 *    GeneralUtils.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_GENERALUTILS_H_
#define DRIVERS_GENERALUTILS_H_
#include <stdint.h>
#include <string>
#include <esp_err.h>
#include <algorithm>
#include <vector>

/**
 * @brief General utilities.
 */
class GeneralUtils {
public:
	static bool        	base64Decode(const std::string& in, std::string* out);
	static bool        	base64Encode(const std::string& in, std::string* out);
	static void        	dumpInfo();
	static bool        	endsWith(std::string str, char c);
	static const char* 	errorToString(esp_err_t errCode);
	static void        	hexDump(const uint8_t* pData, uint32_t length);
	static std::string 	ipToString(uint8_t* ip);
	static std::vector	<std::string> split(std::string source, char delimiter);
	static std::string 	toLower(std::string& value);
	static std::string 	trim(const std::string& str);

};

#endif /* DRIVERS_GENERALUTILS_H_ */