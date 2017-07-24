#ifndef NETWORK_H
#define NETWORK_H

using namespace std;

#include <string>
#include <vector>
#include <map>
#include <stdio.h>

#include "WebServer.h"
#include "API.h"
#include "Device.h"

#define  NVSNetworkWiFiSSID     "WiFiSSID"
#define  NVSNetworkWiFiPassword "WiFiPassword"

struct NetworkDevice {
  DeviceType  Type;
  bool        IsActive;
  string      ID;
  string      IP;
};

class Network_t : public API {
  public:
    string                  WiFiSSID;
    string                  WiFiPassword;
    vector<string>          WiFiList;
    vector<NetworkDevice>   Devices;
    tcpip_adapter_ip_info_t IP;


    Network_t();

    void    Init();
    WebServerResponse_t*  HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params);

  private:

    bool   POSTWiFiSSID(map<string,string>);
    bool   POSTWiFiPassword(map<string,string>);

    string IPToString();
    string ModeToString();
    string WiFiSSIDToString();
    cJSON* APListToJSON();
};

#endif
