using namespace std;

#include <algorithm>
#include <cctype>
#include <locale>

#include "include/Tools.h"

string Tools::ToLower(string Str) {
  string Result = Str;
  transform(Result.begin(), Result.end(), Result.begin(), ::tolower);

  return Result;
}

// trim from start (in place)
void Tools::lTrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void Tools::rTrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void Tools::Trim(std::string &s) {
    lTrim(s);
    rTrim(s);
}
