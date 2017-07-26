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

    NetworkDevice_t *NetworkDevice = DeserializeNetworkDevice(Memory->ArrayGet(NVSNetworkDevicesArray, i));

    if (NetworkDevice->ID != "") {
      NetworkDevice->IsActive = false;
      Devices.push_back(NetworkDevice);
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

          char *Data = SerializeNetworkDevice(Devices.at(i));
          size_t DataLength = strlen(Data);
          Memory->ArrayReplace(NVSNetworkDevicesArray, i, Data, DataLength);
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

    char *Data = SerializeNetworkDevice(Device);
    size_t DataLength = strlen(Data);

    Memory->ArrayAdd(NVSNetworkDevicesArray, Data, DataLength);
    Memory->Commit();
  }
}


char * Network_t::SerializeNetworkDevice(NetworkDevice_t* Item) {
  string SerialString = "";

  if (Item != NULL)
    SerialString = DeviceType_t::ToHexString(Item->TypeHex) + "|" + Item->ID + "|" + Item->IP + "|";

  char *Result = new char[SerialString.length() + 1]; //
  strcpy(Result, SerialString.c_str());

  //ESP_LOGI(tag, "SERIALIZED: %s", Result);

  return Result;
}

NetworkDevice_t* Network_t::DeserializeNetworkDevice(void *Blob) {

  char *Data = (char *)Blob;

  //ESP_LOGI(tag, "DESERIALIZED %s", Data);

  vector<string> StructData = Tools::DivideStrBySymbol(string(Data), '|');
  NetworkDevice_t *Result = new NetworkDevice_t();

  if (StructData.size() > 0) Result->TypeHex = atoi(StructData[0].c_str());
  if (StructData.size() > 1) Result->ID = StructData[1];
  if (StructData.size() > 2) Result->IP = StructData[2];

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
      cJSON *Root;
    	Root = cJSON_CreateObject();

      cJSON_AddStringToObject(Root, "Mode", ModeToString().c_str());
      cJSON_AddStringToObject(Root, "IP"  , IPToString().c_str());
      cJSON_AddStringToObject(Root, "WiFiSSID", WiFiSSIDToString().c_str());
      cJSON_AddItemToObject  (Root, "Devices", DevicesToJSON());

      Result->Body = string(cJSON_Print(Root));

      cJSON_Delete(Root);
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
        Result->Body = cJSON_Print(APListToJSON());
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

cJSON* Network_t::APListToJSON() {

  cJSON *Helper;

  vector<const char *> WiFiListChar = {};

  for (auto& WiFiItem : WiFiList)
    WiFiListChar.push_back(WiFiItem.c_str());

  Helper = cJSON_CreateStringArray(WiFiListChar.data(), WiFiListChar.size());

  return Helper;
}

cJSON* Network_t::DevicesToJSON() {

  cJSON *Result, *Helper;

  Result = cJSON_CreateArray();

  //  for (int i; i < Devices->length(); i++) {

  for (auto& Device: Devices) {
    Helper = cJSON_CreateObject();

    cJSON_AddStringToObject(Helper, "Type", DeviceType_t::ToString(Device->TypeHex).c_str());
    cJSON_AddStringToObject(Helper, "ID", Device->ID.c_str());
    cJSON_AddStringToObject(Helper, "IP", Device->IP.c_str());
    cJSON_AddStringToObject(Helper, "IsActive", (Device->IsActive) ? "1" : "0");

    cJSON_AddItemToArray(Result, Helper);
  }

  return Result;
}
