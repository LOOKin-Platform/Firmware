/*
*    Network.cpp
*    Class to handle API /Device
*
*/
#include "Globals.h"
#include "Network.h"

static char tag[] 				= "Network_t";
static string NVSNetworkArea 	= "Network";

Network_t::Network_t() {}

void Network_t::Init() {
	NVS Memory(NVSNetworkArea);

	JSON JSONObject(Memory.GetString(NVSNetworkWiFiSettings));
	WiFiSettings = JSONObject.GetItems(NULL, true);

	// Load saved network devices from NVS
	uint8_t ArrayCount = Memory.ArrayCount(NVSNetworkDevicesArray);

	for (int i=0; i < ArrayCount; i++) {
		NetworkDevice_t NetworkDevice = DeserializeNetworkDevice(Memory.StringArrayGet(NVSNetworkDevicesArray, i));

		if (NetworkDevice.ID != 0) {
			NetworkDevice.IsActive = false;
			Devices.push_back(NetworkDevice);
		}
	}
}

NetworkDevice_t Network_t::GetNetworkDeviceByID(uint32_t ID) {
	for (int i=0; i < Devices.size(); i++)
		if (Devices[i].ID == ID)
			return Devices[i];

	return NetworkDevice_t();
}

void Network_t::SetNetworkDeviceFlagByIP(string IP, bool Flag) {
	for (int i=0; i < Devices.size(); i++)
		if (Devices[i].IP == IP) {
			Devices[i].IsActive = Flag;
			return;
		}
}

void Network_t::DeviceInfoReceived(string ID, string Type, string IP, string ScenariosVersion, string StorageVersion) {
	ESP_LOGD(tag, "DeviceInfoReceived");

	NVS Memory(NVSNetworkArea);

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
				Memory.StringArrayReplace(NVSNetworkDevicesArray, i, Data);

				Memory.Commit();
			}

			Devices.at(i).IsActive = true;
		}
		else if (Devices.at(i).IP == IP)
			Devices.at(i).IsActive = false;
	}

	// Add new device if no found in routing table
	if (!isIDFound) {
		NetworkDevice_t NetworkDevice;

		NetworkDevice.TypeHex   = TypeHex;
		NetworkDevice.ID        = Converter::UintFromHexString<uint32_t>(ID);
		NetworkDevice.IP        = IP;
		NetworkDevice.IsActive  = true;

		Devices.push_back(NetworkDevice);

		Memory.StringArrayAdd(NVSNetworkDevicesArray, SerializeNetworkDevice(NetworkDevice));
		Memory.Commit();
	}
}

bool Network_t::WiFiConnect(string SSID) {
	if (SSID != "" && WiFiSettings.count(SSID) == 0)
		return false;

	for (auto &WiFiScannedItem : WiFiScannedList) {
		if (SSID != "") {
			if (WiFiScannedItem.getSSID() == SSID) {
				 WiFi.ConnectAP(SSID, WiFiSettings[SSID], WiFiScannedItem.getChannel());
				 return true;
			}
		}
		else if (WiFiSettings.count(WiFiScannedItem.getSSID()) > 0) {
			 WiFi.ConnectAP(WiFiScannedItem.getSSID(), WiFiSettings[WiFiScannedItem.getSSID()], WiFiScannedItem.getChannel());
			 return true;
		}
	}

	return false;
}

string Network_t::SerializeNetworkDevice(NetworkDevice_t Item) {
	JSON JSONObject;

	JSONObject.SetItems(vector<pair<string,string>>({
		make_pair("TypeHex"	, Converter::ToHexString(Item.TypeHex,2)),
		make_pair("ID"		, Converter::ToHexString(Item.ID,8)),
		make_pair("IP"		, Item.IP)
	}));

	return JSONObject.ToString();
}

NetworkDevice_t Network_t::DeserializeNetworkDevice(string Data) {
	NetworkDevice_t Result;

	JSON JSONObject(Data);

	if (!(JSONObject.GetItem("ID").empty()))      Result.ID      = Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("ID"));
	if (!(JSONObject.GetItem("IP").empty()))      Result.IP      = JSONObject.GetItem("IP");
	if (!(JSONObject.GetItem("TypeHex").empty())) Result.TypeHex = (uint8_t)strtol((JSONObject.GetItem("TypeHex")).c_str(), NULL, 16);

	Result.IsActive = false;
	return Result;
}

void Network_t::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
	// обработка GET запроса - получение данных
	if (Type == QueryType::GET) {
		// Запрос JSON со всеми параметрами
		if (URLParts.size() == 0) {
			JSON JSONObject;

			JSONObject.SetItems(vector<pair<string,string>>({
				make_pair("Mode"		, ModeToString()),
				make_pair("IP"			, IPToString()),
				make_pair("CurrentSSID"	, WiFiCurrentSSIDToString())
			}));

			vector<string> WiFiSettingsVector = vector<string>();
			for (auto &Item : WiFiSettings)
				WiFiSettingsVector.push_back(Item.first);

			JSONObject.SetStringArray("SavedSSID", WiFiSettingsVector);

			vector<map<string,string>> NetworkMap = vector<map<string,string>>();
			for (auto& NetworkDevice: Devices)
				NetworkMap.push_back({
				{"Type"     , DeviceType_t::ToString(NetworkDevice.TypeHex)},
				{"ID"       , Converter::ToHexString(NetworkDevice.ID,8)},
				{"IP"       , NetworkDevice.IP},
				{"IsActive" , (NetworkDevice.IsActive == true) ? "1" : "0" }
			});

			JSONObject.SetObjectsArray("Map", NetworkMap);

			Result.Body = JSONObject.ToString();
		}

		// Запрос конкретного параметра или команды секции API
		if (URLParts.size() == 1)
		{
			// Подключение к заданной точке доступа
			if (URLParts[0] == "connect") {
				if (!WiFiConnect()) {
					Result.ResponseCode = WebServer_t::Response::CODE::ERROR;
					Result.Body = "{\"success\" : \"false\"}";
				}
			}

			if (URLParts[0] == "mode") {
				Result.Body = ModeToString();
				Result.ContentType = WebServer_t::Response::TYPE::PLAIN;
			}

			if (URLParts[0] == "ip") {
				Result.Body = IPToString();
				Result.ContentType = WebServer_t::Response::TYPE::PLAIN;
			}

			if (URLParts[0] == "currentssid") {
				Result.Body = WiFiCurrentSSIDToString();

				if (Result.Body == "")
					Result.ResponseCode = WebServer_t::Response::CODE::INVALID;

				Result.ContentType = WebServer_t::Response::TYPE::PLAIN;
			}

			if (URLParts[0] == "scannedssidlist") {
				vector<string> SSIDList = vector<string>();

				for (auto &WiFiScannedItem : WiFiScannedList)
					SSIDList.push_back(WiFiScannedItem.getSSID());

				Result.Body = JSON::CreateStringFromVector(SSIDList);
				Result.ContentType = WebServer_t::Response::TYPE::JSON;
			}
		}

		if (URLParts.size() == 2) {
			if (URLParts[0] == "connect") {
				if (!WiFiConnect(URLParts[1])) {
					Result.ResponseCode = WebServer_t::Response::CODE::ERROR;
					Result.Body = "{\"success\" : \"false\"}";
				}
			}
		}
	}

	// POST запрос - сохранение и изменение данных
	if (Type == QueryType::POST)
	{
		if (URLParts.size() == 0)
		{
			if (Params["wifissid"] == "" || Params["wifipassword"] == "") {
				Result.SetFail();
				return;
			}

			if (WiFiSettings.size() >= Settings.WiFi.SavedAPCount && WiFiSettings.count(Params["wifissid"]) == 0)
				WiFiSettings.erase(WiFiSettings.cbegin());

			WiFiSettings[Params["wifissid"]] = Params["wifipassword"];

			JSON JSONObject;
			JSONObject.SetItems(WiFiSettings);

			NVS Memory(NVSNetworkArea);
			Memory.SetString(NVSNetworkWiFiSettings, JSONObject.ToString());
			Memory.Commit();

			Result.SetSuccess();
		}
	}
}

string Network_t::ModeToString() {
	return (WiFi_t::GetMode() == WIFI_MODE_AP_STR) ? "AP" : "Client";
}

string Network_t::IPToString() {
	if ((WiFi_t::GetMode() == "WIFI_MODE_AP"))
		IP = WiFi.getApIpInfo();

	if ((WiFi_t::GetMode() == "WIFI_MODE_STA"))
		IP = WiFi.getStaIpInfo();

	return inet_ntoa(IP);
}

string Network_t::WiFiCurrentSSIDToString() {
	return WiFi_t::getSSID();
}
