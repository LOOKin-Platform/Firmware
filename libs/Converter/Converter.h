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
#include <math.h>

#include <esp_log.h>
#include <esp_netif_ip_addr.h>
#include <lwip/inet.h>

#include <esp_err.h>
//#include <nvs.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

//#include "Settings.h"

class Converter {
	public:
		static string			ToLower(string);
		static string			ToUpper(string);

		static void				FindAndReplace(string&, string const&, string const&);
		static void				FindAndRemove (string &, string);

		static void				lTrim(string &);  // left trim
		static void				rTrim(string &);  // right trim
		static void				Trim(string &);   // trim

		template <typename T>
		static string			ToString(T);

		template <typename T>
		static string			ToString(T, uint8_t);

		static string			ToHexString(uint64_t, size_t);
		static string			ToASCII(string HexString);

		static string			ToUTF16String(uint16_t SrcString[], uint8_t Size);
		static vector<uint16_t>	ToUTF16Vector(string SrcString);


		static float			ToFloat (string);
		static uint8_t			ToUint8 (string);
		static uint16_t			ToUint16(string);
		static uint32_t			ToUint32(string);
		static int32_t 			ToInt32	(string);

		static bool				Sign(int32_t);

		template <typename T>
		static T				UintFromHexString(string);

		template <typename T>
		static T				UintFromHex(T);

		template <typename T>
		static T				InterpretHexAsDec(T);

		static vector<string> 	StringToVector(string SourceStr, string Delimeter);
		static string			VectorToString(const vector<string>& Strings, const char* Delimeter);

		static string			StringURLDecode(string);
		static string			StringURLEncode(string);

		static void				StringMove(string &Destination, string &Src);
		static void				NormalizeHexString(string &src, uint8_t Length);

		static uint32_t			IPToUint32(esp_netif_ip_info_t);

		static uint8_t			ReverseBits(uint8_t);

		static uint16_t			CRC16(vector<uint8_t> &, uint16_t Start, uint16_t Length);

		static const char* 		ErrorToString(esp_err_t errCode);

		static uint16_t			VoltageFromADC(uint16_t Value, uint8_t R1, uint8_t R2);

		static bool				StartsWith(string &Src, string WhatToFind);

		static uint8_t			SumBytes(const uint8_t * const start, const uint16_t length, const uint8_t init = 0);

		static uint8_t 			Uint8ToBcd(const uint8_t integer);
		static uint8_t			BcdToUint8(const uint8_t bcd);

		static string			GenerateRandomString(uint8_t Size);

		static bool				IsStringContainsOnlyDigits(string &Str);

};
#endif
