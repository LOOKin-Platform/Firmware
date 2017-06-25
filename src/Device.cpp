using namespace std;

#include <sstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iterator>
#include <cJSON.h>

#include "drivers/NVS/NVS.h"

#include "include/Device.h"
#include "include/Switch_str.h"

Device_t::Device_t()
{
  NVSAreaName = "Device";

  Status = DeviceStatus::RUNNING;

  NVS Memory(NVSAreaName);

  string TypeStr = Memory.GetString("Type");
  if (!TypeStr.empty())
  {
      if (TypeStr == "Plug")
        Type = DeviceType::PLUG;
  }
  else
  {
    Type = DeviceType::PLUG;
    Memory.SetString("Type", "Plug");
  }

  string IDStr = Memory.GetString("ID");
  if (!IDStr.empty())
    ID = IDStr;
  else
  {
    ID = "ABCD1234"; // генерация ID
    Memory.SetString("ID", ID);
  }

  string NameStr = Memory.GetString("Name");
  Name = (!NameStr.empty()) ? NameStr : "";

  string PowerModeStr = Memory.GetString("PowerMode");
  if (!PowerModeStr.empty())
    PowerMode = (PowerModeStr == "Battery") ? DevicePowerMode::BATTERY : DevicePowerMode::CONST;
  else
  {
    PowerMode = DevicePowerMode::CONST;
    Memory.SetString("PowerMode", "Const");
  }

  uint8_t PowerModeVoltageInt = Memory.GetInt8Bit("PowerModeVoltage");
  if (PowerModeVoltageInt > 3)
  {
      PowerModeVoltage = PowerModeVoltageInt;
  }
  else
  {
    switch (Type) {
      case DeviceType::PLUG: PowerModeVoltage = 220;
    }
    Memory.SetInt8Bit("PowerModeVoltage", PowerModeVoltage);
  }

  string FirmwareVersionStr = Memory.GetString("FirmwareVersion");
  if (!FirmwareVersionStr.empty())
    FirmwareVersion = FirmwareVersionStr;
  else
  {
    FirmwareVersion = "0.1";
    Memory.SetString("FirmwareVersion", ID);
  }

  Memory.Commit();
}

string Device_t::HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params) {

  string HandleResult = "";
  NVS Memory(NVSAreaName);

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

      HandleResult = string(cJSON_Print(Root));
      cJSON_Delete(Root);
    }

    // Запрос конкретного параметра
    if (URLParts.size() == 1)
    {
      if (URLParts[0] == "type")            HandleResult = TypeToString();
      if (URLParts[0] == "status")          HandleResult = StatusToString();
      if (URLParts[0] == "id")              HandleResult = IDToString();
      if (URLParts[0] == "name")            HandleResult = NameToString();
      if (URLParts[0] == "powermode")       HandleResult = PowerModeToString();
      if (URLParts[0] == "firmwareversion") HandleResult = FirmwareVersionToString();
    }
  }

  // обработка POST запроса - сохранение и изменение данных
  if (Type == QueryType::POST)
  {
    if (URLParts.size() == 0)
    {
      map<string,string>::iterator NameIterator = Params.find("name");

      if (NameIterator != Params.end())
      {
        Name = Params["name"];
        Memory.SetString("Name", Name);
        Memory.Commit();
      }
    }
  }

  return HandleResult;
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
  return ID;
}

string Device_t::NameToString() {
  return Name;
}

string Device_t::PowerModeToString() {

  ostringstream oss_convert;
  oss_convert << PowerModeVoltage;
  string PowerModeVoltageStr = oss_convert.str();

  return (PowerMode == DevicePowerMode::BATTERY) ? "Battery" : PowerModeVoltageStr + "v";
}

string Device_t::FirmwareVersionToString() {
  return FirmwareVersion;
}
