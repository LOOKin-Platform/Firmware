/*
*    Convert.cpp
*    Class to convert types between each other
*
*/

#include "Converter.h"

string Converter::ToLower(string Str) {
	string Result = Str;
	transform(Result.begin(), Result.end(), Result.begin(), ::tolower);

	return Result;
}

string Converter::ToUpper(string Str) {
	string Result = Str;
	transform(Result.begin(), Result.end(), Result.begin(), ::toupper);

	return Result;
}


void Converter::FindAndReplace(string& source, string const& find, string const& replace) {
    for(string::size_type i = 0; (i = source.find(find, i)) != string::npos;) {
        source.replace(i, find.length(), replace);
        i += replace.length();
    }
}

void Converter::FindAndRemove(string& Source, string Chars) {
	for (unsigned int i = 0; i < Chars.size(); ++i)
		Source.erase (remove(Source.begin(), Source.end(), Chars[i]), Source.end());
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

template <typename T>
string Converter::ToString(T Number) {
	ostringstream Convert;
	Convert << +Number;
	return Convert.str();
}
template string Converter::ToString<int8_t>		(int8_t);
template string Converter::ToString<int16_t>	(int16_t);
template string Converter::ToString<int32_t>	(int32_t);
template string Converter::ToString<uint8_t>	(uint8_t);
template string Converter::ToString<uint16_t>	(uint16_t);
template string Converter::ToString<uint32_t>	(uint32_t);
template string Converter::ToString<uint64_t>	(uint64_t);
template string Converter::ToString<float>		(float);
template string Converter::ToString<double>		(double);

template <typename T>
string Converter::ToString(T Number, uint8_t DigitsNum)  {
	string Result = ToString(Number);

	while (Result.size() < DigitsNum) Result = "0" + Result;

	return Result;
}
template string Converter::ToString<uint8_t>	(uint8_t	, uint8_t);
template string Converter::ToString<uint16_t>	(uint16_t	, uint8_t);
template string Converter::ToString<uint32_t>	(uint32_t	, uint8_t);
template string Converter::ToString<uint64_t>	(uint64_t	, uint8_t);

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

string Converter::ToUTF16String(uint16_t SrcString[], uint8_t Size) {
	//string Result((char *) (intptr_t) SrcString); - legacy mode

	string Result;

	for (int i=0; i < Size ; i++) {
		if (SrcString[i] == 0x0000)
			break;

		Result += "\\u" + ToHexString(SrcString[i],4);
	}

	return Result;
}

vector<uint16_t> Converter::ToUTF16Vector(string SrcString) {
	// legacy mode
	//vector<uint16_t> Result((SrcString.size() + sizeof(uint16_t) - 1) / sizeof(uint16_t));
	//copy_n(SrcString.data(), SrcString.size(), reinterpret_cast<char*>(&Result[0]));

	vector<uint16_t> Result = {};
	vector<string> Items = StringToVector(SrcString, "u");

	while (Items.size()) {
		uint16_t Number = UintFromHexString<uint16_t>(Items.front());

		if (Number!=0x0)
			Result.push_back(Number);

		Items.erase(Items.begin());
	}

	return Result;
}

float Converter::ToFloat(string Str) {
	return atof(Str.c_str());
}

uint8_t Converter::ToUint8(string Str) {
	return atoi(Str.c_str());
}

uint16_t Converter::ToUint16(string Str) {
	return atoi(Str.c_str());
}

uint32_t Converter::ToUint32(string Str) {
	return atoi(Str.c_str());
}

int32_t Converter::ToInt32(string Str) {
	return atoi(Str.c_str());
}

bool Converter::Sign(int32_t Num) {
	return (Num < 0) ? false : true;
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

template <typename T>
T Converter::UintFromHex(T Hex) {
    stringstream stream;

    T Result;

    stream << Hex;
    stream >> std::hex >> Result;

    return Result;
}
template uint8_t  	Converter::UintFromHex<uint8_t> (uint8_t 	Hex);
template uint16_t  	Converter::UintFromHex<uint16_t>(uint16_t 	Hex);

template <typename T>
T Converter::InterpretHexAsDec(T Hex) {
	T Result = 0;
	uint8_t ResultLength = 0;

	while (Hex) {
	  uint8_t Digit = Hex & 0xF;
	  Result = Result + Digit * pow(10,ResultLength++);
	  Hex >>= 4;
	}

	return Result;
}
template uint8_t  	Converter::InterpretHexAsDec<uint8_t> (uint8_t 	Hex);
template uint16_t  	Converter::InterpretHexAsDec<uint16_t>(uint16_t Hex);

vector<string> Converter::StringToVector(string SourceStr, string Delimeter) {
	vector<string> Result = vector<string>();

	if (SourceStr == "") return Result;

	size_t pos = 0;
	while ((pos = SourceStr.find(Delimeter)) != string::npos) {
		if (SourceStr.substr(0,pos) != "")
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

string Converter::StringURLDecode(string SrcString) {
	string ret;
	char ch;
	int i, ii, len = SrcString.length();

	for (i=0; i < len; i++) {
		if(SrcString[i] != '%') {
			ret += (SrcString[i] == '+') ? ' ' : SrcString[i];
		}
		else {
			sscanf(SrcString.substr(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }

    return ret;
}


string Converter::StringURLEncode(string SrcString) {
	ostringstream escaped;
	escaped.fill('0');
	escaped << hex;

	for (string::const_iterator i = SrcString.begin(), n = SrcString.end(); i != n; ++i) {
		string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << uppercase;
		escaped << '%' << setw(2) << int((unsigned char) c);
		escaped << nouppercase;
	}

	return escaped.str();
}

void Converter::StringMove(string &Destination, string &Src) {
	const uint8_t PartLen = 100;

	while (Src.size() > 0) {
		Destination += Src.substr(0, (Src.size() >= PartLen) ? PartLen : Src.size());
		Src = (Src.size() < PartLen) ? "" : Src.substr(PartLen);
	}
}

void Converter::NormalizeHexString(string &Src, uint8_t Length) {
	if (Src.size() > Length) Src = Src.substr(0, Length);

	while (Src.size() < Length)
		Src = "0" + Src;
}


uint32_t Converter::IPToUint32(esp_netif_ip_info_t IP) {
  uint32_t BigEndian    = inet_addr(inet_ntoa(IP));
  uint32_t LittleEndian = ((BigEndian>>24)&0xff) | // move byte 3 to byte 0
                          ((BigEndian<<8)&0xff0000) | // move byte 1 to byte 2
                          ((BigEndian>>8)&0xff00) | // move byte 2 to byte 1
                          ((BigEndian<<24)&0xff000000); // byte 0 to byte 3

  return LittleEndian;
}

uint8_t Converter::ReverseBits(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

/*
 * Name  : CRC-16
 * Poly  : 0x8005    x^16 + x^15 + x^2 + 1
 * Init  : 0xFFFF
 * Revert: true
 * XorOut: 0x0000
 * Check : 0x4B37 ("123456789")
 * MaxLen: 4095 байт (32767 бит) - обнаружение одинарных, двойных, тройных и всех нечетных ошибок
 * https://ru.wikibooks.org/wiki/Реализации_алгоритмов/Циклический_избыточный_код#CRC-16
*/
const unsigned short CRC16Table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t Converter::CRC16(vector<uint8_t> &Data, uint16_t Start, uint16_t Length) {
	unsigned short crc = 0xFFFF;
	for (uint16_t i = Start; i< Start + Length; i++)
        crc = (crc >> 8) ^ CRC16Table[(crc & 0xFF) ^ Data[i]];
    return crc;
}

/**
 * @brief Convert an ESP error code to a string.
 * @param [in] errCode The errCode to be converted.
 * @return A string representation of the error code.
 */

const char* Converter::ErrorToString(esp_err_t errCode) {
	return esp_err_to_name(errCode);
}

uint16_t Converter::VoltageFromADC(uint16_t Value, uint8_t R1, uint8_t R2) {
	uint16_t ADCMax = 4095; // Max value at ADC_WIDTH_BIT_12
	uint16_t ADCMin	= 0;	// Reference voltage at ADC_ATTEN_11db

	uint16_t VoltageMax = 3900;
	uint16_t VoltageMin	= 0;

	Value = VoltageMin + (VoltageMax-VoltageMin)*( (Value)-ADCMin )/(ADCMax-ADCMin);
	Value = Value * ( (R1) + (R2) ) / (R2);

	return Value;
}

/**
 * @brief Check is one string starts with another
 * @param [in] Src String where to find
 * @param [in] WhatToFind what to find
 * @return true if starts and false in else cases
 */

bool Converter::StartsWith(string &Src, string WhatToFind) {
	if (WhatToFind.length() > Src.length())
		return false;

	return (Src.rfind(WhatToFind, 0) == 0);
}


uint8_t Converter::SumBytes(const uint8_t * const start, const uint16_t length, const uint8_t init) {
	uint8_t checksum = init;
	const uint8_t *ptr;

	for (ptr = start; ptr - start < length; ptr++) checksum += *ptr;
	return checksum;
}

uint8_t Converter::Uint8ToBcd(const uint8_t integer) {
	if (integer > 99)
		return 255;

	return ((integer / 10) << 4) + (integer % 10);
}

uint8_t Converter::BcdToUint8(const uint8_t bcd) {
	if (bcd > 0x99)
		return 255;

	return (bcd >> 4) * 10 + (bcd & 0xF);
}

string Converter::GenerateRandomString(uint8_t Size) {
    string tmp_s;

    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    srand( (unsigned) time(NULL) * getpid());

    tmp_s.reserve(Size);

    for (int i = 0; i < Size; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];


    return tmp_s;

}


