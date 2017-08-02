#ifndef NETWORK_H
#define NETWORK_H

/*
  Раздел API /network
*/
#include <string>
#include <vector>
#include <map>
#include <stdio.h>

#include "WebServer.h"
#include "API.h"
#include "Device.h"

using namespace std;

#define  NVSNetworkWiFiSSID     "WiFiSSID"
#define  NVSNetworkWiFiPassword "WiFiPassword"
#define  NVSNetworkDevicesArray "Devices"

struct NetworkDevice_t {
  uint8_t   TypeHex   = 0x00;
  bool      IsActive  = 0;
  uint32_t  ID        = 0;
  string    IP        = "";
};

class Network_t : public API {
  public:
    string                    WiFiSSID;
    string                    WiFiPassword;
    vector<string>            WiFiList;
    vector<NetworkDevice_t>   Devices;
    tcpip_adapter_ip_info_t   IP;

    Network_t();

    void  Init();
    NetworkDevice_t GetNetworkDeviceByID(uint32_t);
    void  DeviceInfoReceived(string Type, string ID, string IP);

    static string SerializeNetworkDevice(NetworkDevice_t);
    static NetworkDevice_t DeserializeNetworkDevice(string);

    void HandleHTTPRequest(WebServerResponse_t* &Result, QueryType Type, vector<string> URLParts, map<string,string> Params);

  private:
    bool   POSTWiFiSSID     (map<string,string>);
    bool   POSTWiFiPassword (map<string,string>);

    string IPToString();
    string ModeToString();
    string WiFiSSIDToString();
};

#endif
