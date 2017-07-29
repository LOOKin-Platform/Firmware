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
  Devices       = vector<NetworkDevice_t *>();
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
    NetworkDevice_t *NetworkDevice = DeserializeNetworkDevice(Memory->StringArrayGet(NVSNetworkDevicesArray, i));
    if (NetworkDevice->ID != "") {
      NetworkDevice->IsActive = false;
      Devices.push_back(NetworkDevice);
    }
    else {
      Memory->StringArrayRemove(NVSNetworkDevicesArray, i);
      Memory->Commit();
    }
  }
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
    if (Devices.at(i)->ID == ID) {
        isIDFound = true;

        // If IP of the device changed - change it and save
        if (Devices.at(i)->IP != IP) {
          Devices.at(i)->IP       = IP;
          Devices.at(i)->IsActive = false;

          string Data = SerializeNetworkDevice(Devices.at(i));
          Memory->StringArrayReplace(NVSNetworkDevicesArray, i, Data);
          Memory->Commit();
        }

        Devices.at(i)->IsActive = true;
    }
  }

  // Add new device if no found in routing table
  if (!isIDFound) {
    NetworkDevice_t *Device = new NetworkDevice_t();

    Device->TypeHex   = TypeHex;
    Device->ID        = ID;
    Device->IP        = IP;
    Device->IsActive  = true;

    Devices.push_back(Device);

    string Data = SerializeNetworkDevice(Device);

    Memory->StringArrayAdd(NVSNetworkDevicesArray, Data);
    Memory->Commit();
  }
}


string Network_t::SerializeNetworkDevice(NetworkDevice_t* Item) {
  string Result = "";

  map<string,string> ItemMap = {
    {"TypeHex"  , DeviceType_t::ToHexString(Item->TypeHex)},
    {"ID"       , Item->ID},
    {"IP"       , Item->IP}
  };

  Result =  JSON_t::JSONString(ItemMap);
  //ESP_LOGI(tag, "SERIALIZED: %s", Result.c_str());

  return Result;
}

NetworkDevice_t* Network_t::DeserializeNetworkDevice(string Data) {
  //ESP_LOGI(tag, "DESERIALIZATION START %s", Data.c_str());

  NetworkDevice_t* Result = new NetworkDevice_t();
  map<string,string> Values = JSON_t::MapFromJsonString(Data);

  if (Values.count("ID"))        Result->ID = Values["ID"];
  if (Values.count("IP"))        Result->IP = Values["IP"];
  if (Values.count("TypeHex"))   Result->TypeHex = (uint8_t)strtol((Values["TypeHex"]).c_str(), NULL, 16);

  Result->IsActive = false;

  return Result;
}

WebServerResponse_t* Network_t::HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params) {
  ESP_LOGD(tag, "HandleHTTPRequest");

  WebServerResponse_t* Result = new WebServerResponse_t();

  // обработка GET запроса - получение данных
  if (Type == QueryType::GET)
  {
    // Запрос JSON со всеми параметрами
    if (URLParts.size() == 0)
    {

      StringBuffer sb;
      Writer<StringBuffer> Writer(sb);

      Writer.StartObject();

      map<string,string> JSONMap = {
        {"Mode"     , ModeToString()},
        {"IP"       , IPToString()},
        {"WiFiSSID" , WiFiSSIDToString()}
      };

      JSON_t::AddToObjectFromMap(JSONMap, Writer);

      Writer.Key("Devices");
      Writer.StartArray();
      for (auto& Device: Devices)
        DeviceToJSON(Device, Writer);
      Writer.EndArray();

      Writer.EndObject();

      Result->Body = string(sb.GetString());
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

      if (URLParts[0] == "aplist") {
        StringBuffer sb;
        Writer<StringBuffer> Writer(sb);

        Writer.StartArray();
        for (auto& WiFiItem : WiFiList)
          Writer.String(WiFiItem.c_str());
        Writer.EndArray();

        Result->Body = string(sb.GetString());
        Result->ContentType = WebServerResponse_t::TYPE::JSON;
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
  return Result;

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

void Network_t::DeviceToJSON(NetworkDevice_t *NetworkDevice, Writer<StringBuffer> &Writer) {

  map<string,string> JSONMap = {
    {"Type"     , DeviceType_t::ToString(NetworkDevice->TypeHex)},
    {"ID"       , NetworkDevice->ID},
    {"IP"       , NetworkDevice->IP},
    {"IsActive" , (NetworkDevice->IsActive == true) ? "1" : "0" }
  };

  JSON_t::CreateObjectFromMap(JSONMap, Writer);

  return;
}
