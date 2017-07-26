#ifndef NETWORK_H
#define NETWORK_H

/*
  Раздел API /network
*/

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
#define  NVSNetworkDevicesArray "Devices"

struct NetworkDevice_t {
  uint8_t   TypeHex   = 0x00;
  bool      IsActive  = 0;
  string    ID        = "";
  string    IP        = "";
};

class Network_t : public API {
  public:
    string                    WiFiSSID;
    string                    WiFiPassword;
    vector<string>            WiFiList;
    vector<NetworkDevice_t *> Devices;
    tcpip_adapter_ip_info_t   IP;

    Network_t();

    void    Init();

    void  DeviceInfoReceived(string Type, string ID, string IP);

    static char * SerializeNetworkDevice(NetworkDevice_t *);
    static NetworkDevice_t* DeserializeNetworkDevice(void *);

    WebServerResponse_t*  HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params);

  private:
    bool   POSTWiFiSSID(map<string,string>);
    bool   POSTWiFiPassword(map<string,string>);

    string IPToString();
    string ModeToString();
    string WiFiSSIDToString();
    cJSON* APListToJSON();
    cJSON* DevicesToJSON();
};

#endif
