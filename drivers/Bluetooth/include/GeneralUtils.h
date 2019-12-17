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
		static bool        Base64Decode(const std::string& in, std::string* out);
		static bool        Base64Encode(const std::string& in, std::string* out);
		static void        DumpInfo();
		static bool        EndsWith(std::string str, char c);
		static const char* ErrorToString(esp_err_t errCode);
		static const char* WifiErrorToString(uint8_t value);
		static void        HexDump(const uint8_t* pData, uint32_t length);
		static std::string IPToString(uint8_t* ip);
		static std::vector<std::string> Split(std::string source, char delimiter);
		static std::string ToLower(std::string& value);
		static std::string Trim(const std::string& str);

};

#endif /* COMPONENTS_CPP_UTILS_GENERALUTILS_H_ */
