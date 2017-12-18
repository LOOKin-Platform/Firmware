/*
*    Device.cpp
*    Class to handle API /Device
*
*/
#include "Globals.h"
#include "Device.h"

static char tag[] = "Device_t";
static string NVSDeviceArea = "Device";

map<uint8_t, string> DeviceType_t::TypeMap = {
  { DEVICE_TYPE_PLUG_HEX, DEVICE_TYPE_PLUG_STRING }
};

DeviceType_t::DeviceType_t(string TypeStr) {
  for(auto const &TypeItem : TypeMap) {
    if (Converter::ToLower(TypeItem.second) == Converter::ToLower(TypeStr))
      Hex = TypeItem.first;
  }
}

DeviceType_t::DeviceType_t(uint8_t Type) {
  Hex = Type;
}

bool DeviceType_t::IsBattery() {
  return (Hex > 0x7F) ? true : false; // First bit in device type describe is it battery or not
}

string DeviceType_t::ToString() {
  return ToString(Hex);
}

string DeviceType_t::ToHexString() {
  return Converter::ToHexString(Hex,2);
}

string DeviceType_t::ToString(uint8_t Hex) {
  string Result = "";

  map<uint8_t, string>::iterator it = TypeMap.find(Hex);
  if (it != TypeMap.end()) {
    Result = TypeMap[Hex];
  }

  return Result;
}

Device_t::Device_t() {
    ESP_LOGD(tag, "Constructor");

    Type              = new DeviceType_t(DEVICE_TYPE_PLUG_HEX);
    Status            = DeviceStatus::RUNNING;
    ID                = 0;
    Name              = "";
    PowerMode         = DevicePowerMode::CONST;
    PowerModeVoltage  = 220;
    FirmwareVersion   = FIRMWARE_VERSION;
    Temperature       = 0;
}

void Device_t::Init() {
  ESP_LOGD(tag, "Init");

  NVS *Memory = new NVS(NVSDeviceArea);

  ID = Memory->GetUInt32Bit(NVSDeviceID);
  if (ID == 0) {
    ID = GenerateID();
    Memory->SetUInt32Bit(NVSDeviceID, ID);
  }

  Name = Memory->GetString(NVSDeviceName);

  PowerMode = (Type->IsBattery()) ? DevicePowerMode::BATTERY : DevicePowerMode::CONST;

  uint8_t PowerModeVoltageInt = Memory->GetInt8Bit(NVSDevicePowerModeVoltage);
  if (PowerModeVoltageInt > +3) {
      PowerModeVoltage = PowerModeVoltageInt;
  }
  else {
    switch (Type->Hex) {
      case DEVICE_TYPE_PLUG_HEX: PowerModeVoltage = +220;
    }
    Memory->SetInt8Bit(NVSDevicePowerModeVoltage, +PowerModeVoltage);
  }

  Memory->Commit();
  delete Memory;
}

void Device_t::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
  // обработка GET запроса - получение данных
  if (Type == QueryType::GET) {
    // Запрос JSON со всеми параметрами
    if (URLParts.size() == 0) {

      JSON JSONObject;
      JSONObject.SetItems({
        {"Type"             , TypeToString()},
        {"Status"           , StatusToString()},
        {"ID"               , IDToString()},
        {"Name"             , NameToString()},
        {"Time"             , Time::UnixtimeString()},
        {"Timezone"         , Time::TimezoneStr()},
        {"PowerMode"        , PowerModeToString()},
        {"Firmware"         , FirmwareVersionToString()},
        {"Temperature"      , TemperatureToString()}
      });

      Result.Body = JSONObject.ToString();
    }

    // Запрос конкретного параметра
    if (URLParts.size() == 1) {
      if (URLParts[0] == "type")            Result.Body = TypeToString();
      if (URLParts[0] == "status")          Result.Body = StatusToString();
      if (URLParts[0] == "id")              Result.Body = IDToString();
      if (URLParts[0] == "name")            Result.Body = NameToString();
      if (URLParts[0] == "time")            Result.Body = Time::UnixtimeString();
      if (URLParts[0] == "timezone")        Result.Body = Time::TimezoneStr();
      if (URLParts[0] == "powermode")       Result.Body = PowerModeToString();
      if (URLParts[0] == "firmware")        Result.Body = FirmwareVersionToString();
      if (URLParts[0] == "temperature")     Result.Body = TemperatureToString();

      Result.ContentType = WebServer_t::Response::TYPE::PLAIN;
    }
  }

  // обработка POST запроса - сохранение и изменение данных
  if (Type == QueryType::POST) {
    if (URLParts.size() == 0) {
      bool isNameSet            = POSTName(Params);
      bool isTimeSet            = POSTTime(Params);
      bool isTimezoneSet        = POSTTimezone(Params);
      bool isFirmwareVersionSet = POSTFirmwareVersion(Params, Result);

      if ((isNameSet || isTimeSet || isTimezoneSet || isFirmwareVersionSet) && Result.Body == "")
        Result.Body = "{\"success\" : \"true\"}";
    }

    if (URLParts.size() == 2) {
      if (URLParts[0] == "firmware") {
        Result.Body = "{\"success\" : \"true\"}";

        if (URLParts[1] == "start")
          OTA::ReadStarted();

        if (URLParts[1] == "write") {
          if (Params.count("data") == 0) {
            Result.ResponseCode = WebServer_t::Response::INVALID;
            Result.Body = "{\"success\" : \"false\" , \"Error\": \"Writing data failed\"}";
            return;
          }

          string HexData = Params["data"];
          uint8_t *BinaryData = new uint8_t[HexData.length() / 2];

          for (int i=0; i < HexData.length(); i=i+2)
            BinaryData[i/2] = Converter::UintFromHexString<uint8_t>(HexData.substr(i, 2));

          if (!OTA::ReadBody(reinterpret_cast<char*>(BinaryData), HexData.length() / 2)) {
            Result.ResponseCode = WebServer_t::Response::INVALID;
            Result.Body = "{\"success\" : \"false\" , \"Error\": \"Writing data failed\"}";
          }

          delete BinaryData;
        }

        if (URLParts[1] == "finish")
          OTA::ReadFinished();
      }
    }
  }
}

// Генерация ID на основе MAC-адреса чипа
uint32_t Device_t::GenerateID() {

  uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_AP, mac);

  stringstream MacString;
  MacString << std::dec << (int) mac[0] << (int) mac[1] << (int) mac[2] << (int) mac[3] << (int) mac[4] << (int) mac[5];

  hash<std::string> hash_fn;
  size_t str_hash = hash_fn(MacString.str());

  stringstream Hash;
  Hash << std::uppercase << std::hex << (int)str_hash;

  return Converter::UintFromHexString<uint32_t>(Hash.str());
}

bool Device_t::POSTName(map<string,string> Params) {
  map<string,string>::iterator NameIterator = Params.find("name");

  if (NameIterator != Params.end())
  {
    Name = Params["name"];

    NVS *Memory = new NVS(NVSDeviceArea);
    Memory->SetString(NVSDeviceName, Name);
    Memory->Commit();
    delete Memory;

    return true;
  }

  return false;
}

bool Device_t::POSTTime(map<string,string> Params) {
  if (Params.count("time") > 0) {
    Time::SetTime(Params["time"]);
    return true;
  }

  return false;
}

bool Device_t::POSTTimezone(map<string,string> Params) {
  if (Params.count("timezone") > 0) {
    Time::SetTimezone(Params["timezone"]);
    return true;
  }

  return false;
}

bool Device_t::POSTFirmwareVersion(map<string,string> Params, WebServer_t::Response& Response) {
  if (Params.count("firmware") == 0)
    return false;

  if (Converter::ToFloat(FIRMWARE_VERSION) > Converter::ToFloat(Params["firmware"])) {
    Response.ResponseCode = WebServer_t::Response::CODE::OK;
    Response.Body = "{\"success\" : \"false\" , \"Message\": \"Attempt to update to the old version\"}";
    return false;
  }

  if (Status == DeviceStatus::UPDATING) {
    Response.ResponseCode = WebServer_t::Response::CODE::ERROR;
    Response.Body = "{\"success\" : \"false\" , \"Error\": \"The update process has already been started\"}";
    return false;
  }

  if (WiFi_t::getMode() != WIFI_MODE_STA_STR) {
    Response.ResponseCode = WebServer_t::Response::CODE::ERROR;
    Response.Body = "{\"success\" : \"false\" , \"Error\": \"Device is not connected to the Internet\"}";
    return false;
  }

  string UpdateFilename = "firmware.bin";

  if (Params.count("filename") > 0)
    UpdateFilename = Params["filename"];

  OTA::Update(OTA_URL_PREFIX + Params["firmware"] + "/" + UpdateFilename);

  Device->Status = DeviceStatus::UPDATING;

  Response.ResponseCode = WebServer_t::Response::CODE::OK;
  Response.Body = "{\"success\" : \"true\" , \"Message\": \"Firmware update started\"}";

  return true;
}

string Device_t::TypeToString() {
  return Type->ToString();
}

string Device_t::StatusToString() {
  switch (Status)
  {
    case DeviceStatus::RUNNING  : return "Running"; break;
    case DeviceStatus::UPDATING : return "Updating"; break;
    default: return "";
  }
}

string Device_t::IDToString() {
    return (ID > 0) ? Converter::ToHexString(ID, 8) : "";
}

string Device_t::NameToString() {
  return Name;
}

string Device_t::PowerModeToString() {
  stringstream s;
	s << std::dec << +PowerModeVoltage;

  return (PowerMode == DevicePowerMode::BATTERY) ? "Battery" : s.str() + "v";
}

string Device_t::FirmwareVersionToString() {
  return FirmwareVersion;
}

string Device_t::TemperatureToString() {
  return Converter::ToString(Temperature);
}
