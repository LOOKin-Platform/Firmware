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
#include <utility>
#include <stdio.h>
#include <string.h>

#include "WebServer.h"
#include "WiFi.h"
#include "Memory.h"

using namespace std;

#define  NVSNetworkWiFiSettings	"WiFiSettings"
#define  NVSNetworkDevicesArray "Devices"

struct NetworkDevice_t {
	uint8_t		TypeHex		= 0x00;
	bool		IsActive	= false;
	uint32_t	ID			= 0;
	string		IP			= "";
};

struct WiFiSettingsItem {
	string		SSID 		= "";
	string		Password	= "";

	uint8_t		Channel 	= 255;
	uint32_t	IP			= 0;
	uint32_t	Gateway		= 0;
	uint32_t	Netmask		= 0;
};

class Network_t {
	public:
		vector<WiFiSettingsItem>	WiFiSettings 	= {};
		vector<WiFiAPRecord>		WiFiScannedList = {};
		vector<NetworkDevice_t>		Devices			= {};
		tcpip_adapter_ip_info_t		IP;

		Network_t();

		void 					Init();

		NetworkDevice_t			GetNetworkDeviceByID(uint32_t);
		void					SetNetworkDeviceFlagByIP(string IP, bool Flag);
		void					DeviceInfoReceived(string ID, string Type, string PowerMode, string IP, string ScenariosVersion, string StorageVersion);

		bool					WiFiConnect(string SSID = "", bool DontUseCache = false);
		void					UpdateWiFiIPInfo(string SSID, tcpip_adapter_ip_info_t Data);

		void					AddWiFiNetwork(string SSID, string Password);
		void 					RemoveWiFiNetwork(string SSID);

		void					LoadAccessPointsList();
		void					SaveAccessPointsList();
		string					IPToString();
		vector<string>			GetSavedWiFiList();

		static string 			SerializeNetworkDevice(NetworkDevice_t);
		static NetworkDevice_t 	DeserializeNetworkDevice(string);

		uint32_t 				KeepWiFiTimer 	= 0;

		void HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params);

	private:
		string					ModeToString();
		string					WiFiCurrentSSIDToString();
};

#endif
