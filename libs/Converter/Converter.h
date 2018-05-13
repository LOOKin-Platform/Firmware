/*
*    Convert.h
*    Class to convert types between each other
*
*/

#ifndef LIBS_CONVERTER_H
#define LIBS_CONVERTER_H

using namespace std;

#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <locale>

#include <esp_log.h>
#include <tcpip_adapter.h>
#include <lwip/inet.h>

#include "Settings.h"

class Converter {
	public:
		static string			ToLower(string);

		static void				FindAndReplace(string&, string const&, string const&);
		static void				FindAndRemove (string &, string);

		static void				lTrim(string &);  // left trim
		static void				rTrim(string &);  // right trim
		static void				Trim(string &);   // trim

		static string			ToString(int8_t);
		static string			ToString(uint8_t);
		static string			ToString(uint16_t);
		static string			ToString(uint32_t);
		static string			ToString(uint64_t);
		static string			ToString(double);

		static string			ToHexString(uint64_t, size_t);
		static string			ToASCII(string HexString);

		static string			ToUTF16String(uint16_t SrcString[], uint8_t Size);
		static vector<uint16_t>	ToUTF16Vector(string SrcString);


		static float			ToFloat (string);
		static uint16_t			ToUint16(string);

		template <typename T>
		static T				UintFromHexString(string);

		static vector<string> 	StringToVector		(string SourceStr, string Delimeter);
		static string			VectorToString(const vector<string>& Strings, const char* Delimeter);

		static string			StringURLDecode(string);
		static string			StringURLEncode(string);

		static void				StringMove(string &Destination, string &Src);

		static uint32_t			IPToUint32(tcpip_adapter_ip_info_t);

		static uint8_t			ReverseBits(uint8_t);

		static uint16_t			CRC16(vector<uint8_t>);
};
#endif
