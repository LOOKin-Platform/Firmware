#ifndef TOOLS_H
#define TOOLS_H

using namespace std;

#include <string>

class Tools {
  public:
    static string ToLower(string);

    static void lTrim(string &);  // left trim
    static void rTrim(string &);  // right trim
    static void Trim(string &);   // trim
};

#endif
