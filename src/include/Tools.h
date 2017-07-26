#ifndef TOOLS_H
#define TOOLS_H

using namespace std;

#include <string>
#include <vector>

class Tools {
  public:
    static string   ToLower(string);

    static void     FindAndReplace(string&, string const&, string const&);

    static void     lTrim(string &);  // left trim
    static void     rTrim(string &);  // right trim
    static void     Trim(string &);   // trim

    static string   ToString(uint8_t);
    static vector<string>  DivideStrBySymbol(string, char);
};
#endif
