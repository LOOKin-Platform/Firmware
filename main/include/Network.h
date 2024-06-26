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

#include <esp_netif_ip_addr.h>

#include "WebServer.h"
#include "WiFi.h"
#include "Memory.h"

using namespace std;

#define  NVSNetworkArea			"Network"
#define  NVSNetworkWiFiSettings	"WiFiSettings"
#define  NVSNetworkDevicesArray "Devices"
#define  NVSNetworkAllowOrigin 	"AllowOrigin"


struct NetworkDevice_t {
	uint8_t		TypeHex				= 0x00;
	bool		IsActive			= false;
	uint32_t	ID					= 0;
	string		IP					= "";
	uint16_t	StorageVersion 		= 0;
	uint32_t	AutomationVersion	= 0;
};

struct WiFiSettingsItem {
	string		SSID 		= "";
	string		Password	= "";

	uint8_t		Channel 	= 255;
	uint32_t	IP			= 0;
	uint32_t	Gateway		= 0;
	uint32_t	Netmask		= 0;
};

enum NetworkWiFiBandEnum { K2G4, K5 };
struct WiFiScannedAPListItem {
	string 				SSID				= "";
	uint8_t 			BSSID[6]			= {0};
	wifi_auth_mode_t	SecurityMode		= WIFI_AUTH_MAX;
	uint8_t				Channel 			= 255;
	NetworkWiFiBandEnum	Band 				= K2G4;
	int8_t				RSSI				= 0;
};

class Network_t {
	public:
		vector<WiFiSettingsItem>		WiFiSettings 	= {};
		vector<WiFiScannedAPListItem>	WiFiScannedList = {};
		vector<NetworkDevice_t>			Devices			= {};
		esp_netif_ip_info_t				IP;

		Network_t();

		void 					Init();

		NetworkDevice_t			GetNetworkDeviceByID(uint32_t);
		void					SetNetworkDeviceFlagByIP(string IP, bool Flag);
		void					DeviceInfoReceived(string ID, string Type, string PowerMode, string IP, string ScenariosVersion, string StorageVersion);

		bool					WiFiConnect(string SSID = "", bool DontUseCache = false, bool IsHidden = false);
		void					UpdateWiFiIPInfo(string SSID, esp_netif_ip_info_t Data);

		void					AddWiFiNetwork(string SSID, string Password);
		bool 					RemoveWiFiNetwork(string SSID);

		void					LoadAccessPointsList();
		void					SaveAccessPointsList();
		string					IPToString();
		vector<string>			GetSavedWiFiList();

		static string 			SerializeNetworkDevice(NetworkDevice_t);
		static NetworkDevice_t 	DeserializeNetworkDevice(string);

		uint32_t 				KeepWiFiTimer 	= 0;

		void    				HandleHTTPRequest(WebServer_t::Response &, Query_t &);
		JSON 					RootInfo();

		uint8_t					PoolingNetworkMapReceivedTimer = Settings.Pooling.NetworkMap.DefaultValue;

		void					ImportScannedSSIDList(vector<WiFiAPRecord> ScannedItems);
		void					ImportScannedSSIDList(vector<WiFiScannedAPListItem> ScannedItems);

	private:
		void					WiFiConnectToAPInner(string SSID, string Password, const uint8_t& Channel = 0, bool WaitForConnection = true);

		string					ModeToString();
		string					WiFiCurrentSSIDToString();
};

class NetworkSync {
	public:
		static void 			Start();
		static void 			StorageHistoryQuery();

    	// HTTP Callbacks
    	static void 			ReadStorageStarted (const char *IP);
    	static bool 			ReadStorageBody    (char Data[], int DataLen, const char *IP);
    	static void 			ReadStorageFinished(const char *IP);
    	static void 			StorageAborted     (const char *IP);

	private:
    	static uint8_t			SameQueryCount;
    	static uint16_t			MaxStorageVersion;
    	static uint16_t			ToVersionUpgrade;
    	static string			SourceIP;
    	static string 			Chunk;
};

#endif
