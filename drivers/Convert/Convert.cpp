/*
*    Convert.cpp
*    Class to convert types between each other
*
*/

#include "Convert.h"

string Converter::ToLower(string Str) {
  string Result = Str;
  transform(Result.begin(), Result.end(), Result.begin(), ::tolower);

  return Result;
}

void Converter::FindAndReplace(string& source, string const& find, string const& replace) {
    for(string::size_type i = 0; (i = source.find(find, i)) != string::npos;) {
        source.replace(i, find.length(), replace);
        i += replace.length();
    }
}

// trim from start (in place)
void Converter::lTrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void Converter::rTrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void Converter::Trim(std::string &s) {
    lTrim(s);
    rTrim(s);
}

string Converter::ToString(int8_t Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

string Converter::ToString(uint8_t Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

string Converter::ToString(uint32_t Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

string Converter::ToString(double Number) {
  ostringstream Convert;
  Convert << +Number;
  return Convert.str();
}

string Converter::ToHexString(uint64_t Number, size_t Length) {
  stringstream sstream;
  sstream << std::uppercase << std::setfill('0') << std::setw(Length) << std::hex << (uint64_t)Number;
  return (sstream.str());
}

string Converter::ToASCII(string HexString) {
  if (HexString.length() % 2 != 0)
    HexString = "0" + HexString;

  string Result = "";
  while (HexString.length() > 0) {
    Result.push_back((char)UintFromHexString<uint8_t>(HexString.substr(0, 2)));
    HexString = HexString.substr(2, string::npos);
  }
  
  return Result;
}

float Converter::ToFloat(string Str) {
  return atof(Str.c_str());
}

template <typename T>
T Converter::UintFromHexString(string HexString) {

  if (is_same<T, uint8_t>::value)
    return (uint8_t)Converter::UintFromHexString<uint16_t>(HexString);

  T Output;
  stringstream ss(HexString);
  ss >> std::hex >> Output;

  return +Output;
}
template uint8_t  Converter::UintFromHexString<uint8_t> (string HexString);
template uint16_t Converter::UintFromHexString<uint16_t>(string HexString);
template uint32_t Converter::UintFromHexString<uint32_t>(string HexString);
template uint64_t Converter::UintFromHexString<uint64_t>(string HexString);

vector<string> Converter::StringToVector(string SourceStr, string Delimeter) {
  vector<string> Result;

  size_t pos = 0;
  while ((pos = SourceStr.find(Delimeter)) != string::npos) {
    Result.push_back(SourceStr.substr(0, pos));
    SourceStr.erase(0, pos + Delimeter.length());
  }

  Result.push_back(SourceStr);

  return Result;
}

string Converter::VectorToString(const vector<string>& Strings, const char* Delimeter) {
  switch (Strings.size()) {
      case 0: return "";
      case 1: return Strings[0];
      default:
          ostringstream os;
          copy(Strings.begin(), Strings.end()-1, ostream_iterator<string>(os, Delimeter));
          os << *Strings.rbegin();
          return os.str();
  }
}

uint32_t Converter::IPToUint32(tcpip_adapter_ip_info_t IP) {
  uint32_t BigEndian    = inet_addr(inet_ntoa(IP));
  uint32_t LittleEndian = ((BigEndian>>24)&0xff) | // move byte 3 to byte 0
                          ((BigEndian<<8)&0xff0000) | // move byte 1 to byte 2
                          ((BigEndian>>8)&0xff00) | // move byte 2 to byte 1
                          ((BigEndian<<24)&0xff000000); // byte 0 to byte 3

  return LittleEndian;
}
