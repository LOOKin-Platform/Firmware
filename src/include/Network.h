/*
*    Network.h
*    Class to handle API /Device
*
*/

#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>

#include "WebServer.h"
#include "WiFi.h"
#include "Memory.h"

using namespace std;

#define  NVSNetworkWiFiSettings	"WiFiSettings"
#define  NVSNetworkDevicesArray "Devices"

struct NetworkDevice_t {
  uint8_t   TypeHex   = 0x00;
  bool      IsActive  = false;
  uint32_t  ID        = 0;
  string    IP        = "";
};

class Network_t {
  public:
	map<string,string>		WiFiSettings 	= {};
    vector<string>			WiFiList 		= vector<string>();
    vector<NetworkDevice_t>	Devices			= vector<NetworkDevice_t>();
    tcpip_adapter_ip_info_t	IP;

    Network_t();

    void  Init();
    NetworkDevice_t			GetNetworkDeviceByID(uint32_t);
    void					SetNetworkDeviceFlagByIP(string IP, bool Flag);
    void					DeviceInfoReceived(string ID, string Type, string IP, string ScenariosVersion, string StorageVersion);

    bool					WiFiConnect(string SSID = "");
    string					IPToString();

    static string 			SerializeNetworkDevice(NetworkDevice_t);
    static NetworkDevice_t 	DeserializeNetworkDevice(string);

    void HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params);

  private:
    string					ModeToString();
    string					WiFiCurrentSSIDToString();
};

#endif
