/*
  Класс для работы с API /network
*/

#include "JSON.h"

#include <string.h>

#include "Globals.h"
#include "Network.h"

#include "WiFi/WiFi.h"
#include "NVS/NVS.h"

static char tag[] = "Network_t";
static string NVSNetworkArea = "Network";

Network_t::Network_t() {
  ESP_LOGD(tag, "Constructor");

  WiFiSSID      = "";
  WiFiPassword  = "";
  WiFiList      = vector<string>();
  Devices       = vector<NetworkDevice_t>();
}

void Network_t::Init() {
  ESP_LOGD(tag, "Init");

  NVS *Memory = new NVS(NVSNetworkArea);

  WiFiSSID     = Memory->GetString(NVSNetworkWiFiSSID);
  WiFiPassword = Memory->GetString(NVSNetworkWiFiPassword);

  //Memory->ArrayEraseAll(NVSNetworkDevicesArray);

  // Read info from network scan
  for (WiFiAPRecord APRecord : WiFi->Scan())
      WiFiList.push_back(APRecord.getSSID());

  // Load saved network devices from NVS
  uint8_t ArrayCount = Memory->ArrayCount(NVSNetworkDevicesArray);

  for (int i=0; i < ArrayCount; i++) {
    NetworkDevice_t NetworkDevice = DeserializeNetworkDevice(Memory->StringArrayGet(NVSNetworkDevicesArray, i));
    if (NetworkDevice.ID != 0) {
      NetworkDevice.IsActive = false;
      Devices.push_back(NetworkDevice);
    }
    else {
      Memory->StringArrayRemove(NVSNetworkDevicesArray, i);
      Memory->Commit();
    }
  }

  delete Memory;
}

NetworkDevice_t Network_t::GetNetworkDeviceByID(uint32_t ID) {
  for (int i=0; i < Devices.size(); i++)
    if (Devices[i].ID == ID)
      return Devices[i];

  return NetworkDevice_t();
}


void Network_t::DeviceInfoReceived(string Type, string ID, string IP) {
  ESP_LOGD(tag, "DeviceInfoReceived");

  NVS *Memory = new NVS(NVSNetworkArea);

  bool isIDFound = false;
  uint8_t TypeHex = +0;

  int TypeTmp = atoi(Type.c_str());
  if (TypeTmp < +255)
    TypeHex = +TypeTmp;

  for (int i=0; i < Devices.size(); i++) {
    if (Devices.at(i).ID == Converter::UintFromHexString<uint32_t>(ID)) {
        isIDFound = true;

        // If IP of the device changed - change it and save
        if (Devices.at(i).IP != IP) {
          Devices.at(i).IP       = IP;
          Devices.at(i).IsActive = false;

          string Data = SerializeNetworkDevice(Devices.at(i));
          Memory->StringArrayReplace(NVSNetworkDevicesArray, i, Data);
          Memory->Commit();
        }

        Devices.at(i).IsActive = true;
    }
  }

  // Add new device if no found in routing table
  if (!isIDFound) {
    NetworkDevice_t Device;

    Device.TypeHex   = TypeHex;
    Device.ID        = Converter::UintFromHexString<uint32_t>(ID);
    Device.IP        = IP;
    Device.IsActive  = true;

    Devices.push_back(Device);

    string Data = SerializeNetworkDevice(Device);

    Memory->StringArrayAdd(NVSNetworkDevicesArray, Data);
    Memory->Commit();
  }
}


string Network_t::SerializeNetworkDevice(NetworkDevice_t Item) {
  JSON_t *JSON = new JSON_t();

  JSON->SetParam({
    {"TypeHex"  , Converter::ToHexString(Item.TypeHex,2)},
    {"ID"       , Converter::ToHexString(Item.ID,8) },
    {"IP"       , Item.IP}
  });

  string Result = JSON->ToString();
  delete JSON;

  return Result;
}

NetworkDevice_t Network_t::DeserializeNetworkDevice(string Data) {

  NetworkDevice_t Result;
  JSON_t *JSON = new JSON_t(Data);

  if (!(JSON->GetParam("ID").empty()))      Result.ID      = Converter::UintFromHexString<uint32_t>(JSON->GetParam("ID"));
  if (!(JSON->GetParam("IP").empty()))      Result.IP      = JSON->GetParam("IP");
  if (!(JSON->GetParam("TypeHex").empty())) Result.TypeHex = (uint8_t)strtol((JSON->GetParam("TypeHex")).c_str(), NULL, 16);

  delete JSON;
  Result.IsActive = false;

  return Result;
}

void Network_t::HandleHTTPRequest(WebServerResponse_t* &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
  // обработка GET запроса - получение данных
  if (Type == QueryType::GET) {
    // Запрос JSON со всеми параметрами
    if (URLParts.size() == 0) {

      JSON_t *JSON = new JSON_t();

      JSON->SetParam({
        {"Mode"     , ModeToString()},
        {"IP"       , IPToString()},
        {"WiFiSSID" , WiFiSSIDToString()}
      });

      vector<map<string,string>> NetworkMap = vector<map<string,string>>();
      for (auto& NetworkDevice: Devices)
        NetworkMap.push_back({
          {"Type"     , DeviceType_t::ToString(NetworkDevice.TypeHex)},
          {"ID"       , Converter::ToHexString(NetworkDevice.ID,8)},
          {"IP"       , NetworkDevice.IP},
          {"IsActive" , (NetworkDevice.IsActive == true) ? "1" : "0" }
        });

      JSON->SetMapsArray("Map", NetworkMap);

      Result->Body = JSON->ToString();

      delete JSON;
    }

    // Запрос конкретного параметра или команды секции API
    if (URLParts.size() == 1)
    {
      // Подключение к заданной точке доступа
      if (URLParts[0] == "connect") {
        if (WiFiSSID != "") WiFi->ConnectAP(WiFiSSID, WiFiPassword);
      }

      if (URLParts[0] == "mode") {
        Result->Body = ModeToString();
        Result->ContentType = WebServerResponse_t::TYPE::PLAIN;
      }

      if (URLParts[0] == "ip") {
        Result->Body = IPToString();
        Result->ContentType = WebServerResponse_t::TYPE::PLAIN;
      }

      if (URLParts[0] == "wifissid") {
        Result->Body = WiFiSSIDToString();

        if (Result->Body == "")
          Result->ResponseCode = WebServerResponse_t::CODE::INVALID;

        Result->ContentType = WebServerResponse_t::TYPE::PLAIN;
      }

      if (URLParts[0] == "wifilist") {
        JSON_t *JSON = new JSON_t();
        JSON->SetVector(WiFiList);

        Result->Body = JSON->ToString(true);
        Result->ContentType = WebServerResponse_t::TYPE::JSON;
        delete JSON;
      }
    }
  }

  // POST запрос - сохранение и изменение данных
  if (Type == QueryType::POST)
  {
    if (URLParts.size() == 0)
    {
      bool isSSIDSet      = POSTWiFiSSID(Params);
      bool isPasswordSet  = POSTWiFiPassword(Params);

      if (isSSIDSet || isPasswordSet)
        Result->Body = "{\"success\" : \"true\"}";
    }
  }
}

bool Network_t::POSTWiFiSSID(map<string,string> Params) {

  map<string,string>::iterator WiFiSSIDIterator = Params.find("wifissid");

  if (WiFiSSIDIterator != Params.end())
  {
    WiFiSSID = Params["wifissid"];

    NVS *Memory = new NVS(NVSNetworkArea);
    Memory->SetString(NVSNetworkWiFiSSID, WiFiSSID);
    Memory->Commit();

    return true;
  }

  return false;
}

bool Network_t::POSTWiFiPassword(map<string,string> Params) {

  map<string,string>::iterator WiFiPasswordIterator = Params.find("wifipassword");

  if (WiFiPasswordIterator != Params.end())
  {
    WiFiPassword = Params["wifipassword"];

    NVS *Memory = new NVS(NVSNetworkArea);
    Memory->SetString(NVSNetworkWiFiPassword, WiFiPassword);
    Memory->Commit();

    return true;
  }

  return false;
}


string Network_t::ModeToString() {
  return (WiFi_t::getMode() == WIFI_MODE_AP_STR) ? "AP" : "Client";
}

string Network_t::IPToString() {
  if ((WiFi_t::getMode() == "WIFI_MODE_AP"))
    IP = WiFi->getApIpInfo();

  if ((WiFi_t::getMode() == "WIFI_MODE_STA"))
    IP = WiFi->getStaIpInfo();

  return inet_ntoa(IP);
}

string Network_t::WiFiSSIDToString() {
  return WiFiSSID;
}
