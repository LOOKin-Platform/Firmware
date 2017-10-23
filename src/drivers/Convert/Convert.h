/*
*    Convert.h
*    Class to convert types between each other
*
*/

#ifndef CONVERTER_H
#define CONVERTER_H

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

#include <tcpip_adapter.h>
#include <lwip/inet.h>

class Converter {
  public:
    static string   ToLower(string);

    static void     FindAndReplace(string&, string const&, string const&);

    static void     lTrim(string &);  // left trim
    static void     rTrim(string &);  // right trim
    static void     Trim(string &);   // trim

    static string   ToString(int8_t);
    static string   ToString(uint8_t);
    static string   ToString(uint32_t);
    static string   ToString(double);

    static string   ToHexString(uint64_t, size_t);

    static float    ToFloat(string);

    template <typename T>
    static T        UintFromHexString(string);

    static vector<string> StringToVector(string SourceStr, string Delimeter);
    static string   VectorToString(const vector<string>& Strings, const char* Delimeter);

    static uint32_t IPToUint32(tcpip_adapter_ip_info_t);
};
#endif
