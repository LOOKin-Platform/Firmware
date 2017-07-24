#include <algorithm>
#include <string>
#include <iterator>
#include <iostream>
#include <sstream>

#include <cJSON.h>

#include "Globals.h"
#include "Device.h"
#include "OTA.h"

#include "NVS/NVS.h"
#include "Switch_str.h"

static char tag[] = "Device_t";
static string NVSDeviceArea = "Device";

Device_t::Device_t()
{
    ESP_LOGD(tag, "Constructor");

    Type              = DeviceType::PLUG;
    Status            = DeviceStatus::RUNNING;
    ID                = "";
    Name              = "";
    PowerMode         = DevicePowerMode::CONST;
    PowerModeVoltage  = 220;
    FirmwareVersion   = FIRMWARE_VERSION;

    switch (Type) {
      case DeviceType::PLUG: Name = DEFAULT_NAME_PLUG; break;
    }
}

void Device_t::Init()
{
  ESP_LOGD(tag, "Init");

  NVS *Memory = new NVS(NVSDeviceArea);

  string TypeStr = Memory->GetString(NVSDeviceType);
  if (!TypeStr.empty())
  {
      if (TypeStr == "Plug") Type = DeviceType::PLUG;
  }
  else
  {
    Type = DeviceType::PLUG;
    Memory->SetString(NVSDeviceType, "Plug");
  }

  string IDStr = Memory->GetString(NVSDeviceID);
  if (!IDStr.empty() && IDStr.length() == 8)
    ID = IDStr;
  else
  {
    // Настройка как нового устройства
    ID = GenerateID();
    Memory->SetString(NVSDeviceID, ID);
    Memory->SetString(NVSDeviceName, Name);
    Memory->Commit();
  }

  string NameStr = Memory->GetString(NVSDeviceName);
  Name = (!NameStr.empty()) ? NameStr : "";

  string PowerModeStr = Memory->GetString(NVSDevicePowerMode);
  if (!PowerModeStr.empty())
    PowerMode = (PowerModeStr == "Battery") ? DevicePowerMode::BATTERY : DevicePowerMode::CONST;
  else
  {
    PowerMode = DevicePowerMode::CONST;
    Memory->SetString(NVSDevicePowerMode, "Const");
  }

  uint8_t PowerModeVoltageInt = Memory->GetInt8Bit(NVSDevicePowerModeVoltage);
  if (PowerModeVoltageInt > +3)
  {
      PowerModeVoltage = PowerModeVoltageInt;
  }
  else
  {
    switch (Type)
    {
      case DeviceType::PLUG: PowerModeVoltage = +220;
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
  switch (Type)
  {
    case DeviceType::PLUG: return "Plug"; break;
    default: return "";
  }
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
