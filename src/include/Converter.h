#ifndef CONVERTER_H
#define CONVERTER_H

#include <string>
#include <vector>
#include <bitset>

using namespace std;

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

    static string   ToHexString(uint64_t, size_t);

    template <typename T>
    static T        UintFromHexString(string);

    static vector<string> StringToVector(string SourceStr, string Delimeter);
};
#endif
