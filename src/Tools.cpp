using namespace std;

#include <algorithm>
#include <sstream>
#include <cctype>
#include <locale>

#include "Tools.h"

string Tools::ToLower(string Str) {
  string Result = Str;
  transform(Result.begin(), Result.end(), Result.begin(), ::tolower);

  return Result;
}

void Tools::FindAndReplace(string& source, string const& find, string const& replace)
{
    for(string::size_type i = 0; (i = source.find(find, i)) != string::npos;)
    {
        source.replace(i, find.length(), replace);
        i += replace.length();
    }
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

string Tools::ToString(int8_t Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

string Tools::ToString(uint8_t Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

string Tools::ToString(uint32_t Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

vector<string> Tools::DivideStrBySymbol(string SourceStr, char Divider) {

  vector<string> Result;

  istringstream f(SourceStr);
  string s;

  while (getline(f, s, Divider))
      Result.push_back(s);

  return Result;
}

/*
// case-independent (ci) compare_less binary function
struct CompareNoCase : binary_function<string, string, bool> {
  struct NocaseCompare : public binary_function<unsigned char,unsigned char,bool>
  {
    bool operator() (const unsigned char& c1, const unsigned char& c2) const {
        return tolower (c1) < tolower (c2);
    }
  };
  bool operator() (const string & s1, const string & s2) const {
    return lexicographical_compare
      (s1.begin (), s1.end (),    // source range
      s2.begin (), s2.end (),     // dest range
      NocaseCompare ());         // comparison
  }
};
*/
