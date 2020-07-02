/*
*    Data.h
*    Class to handle API /Data
*
*/

#ifndef DATA_H
#define DATA_H

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

class Data {
	public:

		static map<string,vector<string>> AvaliableFunctions;

		enum Error {
			Ok 					= 0x00,
			EmptyUUIDDevice 	= 0x01,
			DeviceAlreadyExists = 0x10,
			NoSpaceToSaveDevice = 0x11,
			DeviceNotFound		= 0x20,
			SignalsFieldEmpty	= 0x21,
			UnsupportedFunction	= 0x22,
			NoFreeSpace 		= 0x91
		};

		class IRDevice {
			public:
				void ParseJSONItems(map<string, string>);
				IRDevice(string Data = "", string UUID = "");
				IRDevice(map<string,string> Items, string UUID = "");

				string ToString(bool IsShortened = true);

				bool 		IsCorrect();

				string 		UUID 	= "";
				string 		Type 	= "";
				uint32_t	Updated = 0;
				string 		Name	= "";

				map<string,string> Functions = map<string,string>();
		};

		static vector<string> IRDevicesList;

		static void 	Init();

		static void 	HandleHTTPRequest(WebServer_t::Response &, QueryType, vector<string>, map<string,string>, string RequestBody, httpd_req_t *Request = NULL, WebServer_t::QueryTransportType TransportType = WebServer_t::QueryTransportType::WebServer, int MsgID = 0);

		static uint8_t	SaveIRDevicesList();
		static uint8_t	AddToDevicesList(string UUID);
		static uint8_t 	RemoveFromIRDevicesList(string UUID);

		static IRDevice	LoadDevice(string UUID);
		static uint8_t	SaveDevice(IRDevice Device);

		static pair<bool,IRLib>
						LoadFunctionByIndex(string UUID, string Function, uint8_t Index = 0x0, IRDevice DeviceItem = IRDevice());
		static vector<IRLib>
						LoadAllFunctionSignals(string UUID, string Function, IRDevice DeviceItem = IRDevice());

		static uint8_t	SaveFunction(string UUID, string Function, string Item, uint8_t Index, string DeviceType);

		static string 	SerializeIRSignal	(JSON &);
		static pair<bool, IRLib>
						DeserializeIRSignal	(string);

		static bool		CheckIsValidKeyForType(string Type, string Key);

		//static string 	SerializeIRSignal(IRLib &);

};


#endif
