#include <algorithm>
#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <cJSON.h>

#include "Globals.h"
#include "Device.h"
#include "OTA.h"

#include "NVS/NVS.h"
#include "Switch_str.h"

static char tag[] = "Device_t";
static string NVSDeviceArea = "Device";

map<uint8_t, string> DeviceType_t::TypeMap = {
  { DEVICE_TYPE_PLUG_HEX, DEVICE_TYPE_PLUG_STRING }
};

DeviceType_t::DeviceType_t(string TypeStr) {
  for(auto const &TypeItem : TypeMap) {
    if (Tools::ToLower(TypeItem.second) == Tools::ToLower(TypeStr))
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
  return ToHexString(Hex);
}

string DeviceType_t::ToString(uint8_t Hex) {
  string Result = "";

  map<uint8_t, string>::iterator it = TypeMap.find(Hex);
  if (it != TypeMap.end()) {
    Result = TypeMap[Hex];
  }

  return Result;
}

string DeviceType_t::ToHexString(uint8_t Hex) {
  stringstream sstream;
  sstream << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)Hex;
  return (sstream.str());
}

Device_t::Device_t()
{
    ESP_LOGD(tag, "Constructor");

    Type              = new DeviceType_t(DEVICE_TYPE_PLUG_HEX);
    Status            = DeviceStatus::RUNNING;
    ID                = "";
    Name              = "";
    PowerMode         = DevicePowerMode::CONST;
    PowerModeVoltage  = 220;
    FirmwareVersion   = FIRMWARE_VERSION;
}

void Device_t::Init()
{
  ESP_LOGD(tag, "Init");

  NVS *Memory = new NVS(NVSDeviceArea);

  //Type = new DeviceType_t(Memory->GetInt8Bit(NVSDeviceType));

  string IDStr = Memory->GetString(NVSDeviceID);
  if (!IDStr.empty() && IDStr.length() == 8)
    ID = IDStr;
  else
  {
    // Настройка как нового устройства
    ID = GenerateID();
    Memory->SetString(NVSDeviceID, ID);
    Memory->Commit();
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
}

WebServerResponse_t* Device_t::HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params) {
  ESP_LOGD(tag, "HandleHTTPRequest");

  WebServerResponse_t* Result = new WebServerResponse_t();

  // обработка GET запроса - получение данных
  if (Type == QueryType::GET)
  {
    // Запрос JSON со всеми параметрами
    if (URLParts.size() == 0)
    {
      cJSON *Root;
    	Root = cJSON_CreateObject();

      cJSON_AddStringToObject(Root, "Type"            ,TypeToString().c_str());
      cJSON_AddStringToObject(Root, "Status"          ,StatusToString().c_str());
      cJSON_AddStringToObject(Root, "ID"              ,IDToString().c_str());
      cJSON_AddStringToObject(Root, "Name"            ,NameToString().c_str());
      cJSON_AddStringToObject(Root, "PowerMode"       ,PowerModeToString().c_str());
      cJSON_AddStringToObject(Root, "FirmwareVersion" ,FirmwareVersionToString().c_str());

      Result->Body = string(cJSON_Print(Root));
      cJSON_Delete(Root);
    }

    // Запрос конкретного параметра
    if (URLParts.size() == 1)
    {
      if (URLParts[0] == "type")            Result->Body = TypeToString();
      if (URLParts[0] == "status")          Result->Body = StatusToString();
      if (URLParts[0] == "id")              Result->Body = IDToString();
      if (URLParts[0] == "name")            Result->Body = NameToString();
      if (URLParts[0] == "powermode")       Result->Body = PowerModeToString();
      if (URLParts[0] == "firmwareversion") Result->Body = FirmwareVersionToString();

      Result->ContentType = WebServerResponse_t::TYPE::PLAIN;
    }
  }

  // обработка POST запроса - сохранение и изменение данных
  if (Type == QueryType::POST)
  {
    if (URLParts.size() == 0)
    {
      bool isNameSet            = POSTName(Params);
      bool isFirmwareVersionSet = POSTFirmwareVersion(Params, *Result);

      if ((isNameSet || isFirmwareVersionSet) && Result->Body == "")
        Result->Body = "{\"success\" : \"true\"}";
    }
  }

  return Result;
}

// Генерация ID на основе MAC-адреса чипа
string Device_t::GenerateID() {

  uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_AP, mac);

  stringstream MacString;
  MacString << std::dec << (int) mac[0] << (int) mac[1] << (int) mac[2] << (int) mac[3] << (int) mac[4] << (int) mac[5];

  hash<std::string> hash_fn;
  size_t str_hash = hash_fn(MacString.str());

  stringstream Hash;
  Hash << std::uppercase << std::hex << (int)str_hash;

  return Hash.str();
}

bool Device_t::POSTName(map<string,string> Params) {
  map<string,string>::iterator NameIterator = Params.find("name");

  if (NameIterator != Params.end())
  {
    Name = Params["name"];

    NVS *Memory = new NVS(NVSDeviceArea);
    Memory->SetString(NVSDeviceName, Name);
    Memory->Commit();

    return true;
  }

  return false;
}

bool Device_t::POSTFirmwareVersion(map<string,string> Params, WebServerResponse_t& Response) {

  map<string,string>::iterator FirmwareVersionIterator = Params.find("firmwareversion");

  if (FirmwareVersionIterator != Params.end()) {
    string UpdateVersion = Params["firmwareversion"];

    if (Status == DeviceStatus::RUNNING) {
        if (WiFi_t::getMode() == WIFI_MODE_STA_STR) {

          string UpdateFilename = "firmware.bin";

          map<string,string>::iterator FilenameIterator = Params.find("Filename");
          if (FilenameIterator != Params.end())
            UpdateFilename = Params["Filename"];

          OTA_t *OTA = new OTA_t();
          OTA->Update(OTA_URL_PREFIX + UpdateVersion + "/" + UpdateFilename);

          Device->Status = DeviceStatus::UPDATING;

          Response.ResponseCode = WebServerResponse_t::CODE::OK;
          Response.Body = "{\"success\" : \"true\" , \"Message\": \"Firmware update started\"}";

          return true;
        }
        else {
          Response.ResponseCode = WebServerResponse_t::CODE::ERROR;
          Response.Body = "{\"success\" : \"false\" , \"Error\": \"Device is not connected to the Internet\"}";
        }
    }
    else {
      Response.ResponseCode = WebServerResponse_t::CODE::ERROR;
      Response.Body = "{\"success\" : \"false\" , \"Error\": \"The update process has already been started\"}";
    }
  }

  return false;
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
    return (ID.length() == 8) ? ID : "";
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
