#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>

#include <Tools.h>

using namespace std;

class Tools {
  public:
    static string   ToLower(string);

    static void     FindAndReplace(string&, string const&, string const&);

    static void     lTrim(string &);  // left trim
    static void     rTrim(string &);  // right trim
    static void     Trim(string &);   // trim

    static string   ToString(int8_t);
    static string   ToString(uint8_t);
    static string   ToString(uint32_t);

    static vector<string>  DivideStrBySymbol(string, char);
};
#endif
