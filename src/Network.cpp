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
	LoadAccessPointsList();

	NVS Memory(NVSNetworkArea);

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

void Network_t::DeviceInfoReceived(string ID, string Type, string PowerMode, string IP, string AutomationVersion, string StorageVersion) {
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
				Devices.at(i).IP      		 	= IP;
				Devices.at(i).IsActive 			= false;

				string Data = SerializeNetworkDevice(Devices.at(i));
				Memory.StringArrayReplace(NVSNetworkDevicesArray, i, Data);

				Memory.Commit();
			}

			Devices.at(i).StorageVersion 	= Converter::UintFromHexString<uint16_t>(StorageVersion);
			Devices.at(i).AutomationVersion = Converter::UintFromHexString<uint32_t>(AutomationVersion);

			Devices.at(i).IsActive = true;
		}
		else if (Devices.at(i).IP == IP)
			Devices.at(i).IsActive = false;
	}

	// Add new device if no found in routing table
	if (!isIDFound) {
		NetworkDevice_t NetworkDevice;

		NetworkDevice.TypeHex   		= TypeHex;
		NetworkDevice.ID        		= Converter::UintFromHexString<uint32_t>(ID);
		NetworkDevice.IP        		= IP;
		NetworkDevice.IsActive  		= true;
		NetworkDevice.StorageVersion 	= Converter::UintFromHexString<uint16_t>(StorageVersion);
		NetworkDevice.AutomationVersion = Converter::UintFromHexString<uint32_t>(AutomationVersion);

		Devices.push_back(NetworkDevice);

		Memory.StringArrayAdd(NVSNetworkDevicesArray, SerializeNetworkDevice(NetworkDevice));
		Memory.Commit();
	}

	PoolingNetworkMapReceivedTimer = Settings.Pooling.NetworkMap.ActionsDelay;
}

bool Network_t::WiFiConnect(string SSID, bool DontUseCache, bool IsHidden) {
	string Password = "";
	for (auto &item : WiFiSettings)
		if (Converter::ToLower(item.SSID) == Converter::ToLower(SSID)) {
			SSID = item.SSID;
			Password = item.Password;
			break;
		}

	ESP_LOGI("WiFiConnect", "SSID: %s", SSID.c_str());

	if (SSID != "" && Password == "")
		return false;

	if (SSID != "" && Converter::ToLower(SSID) == Converter::ToLower(WiFi.GetSSID()))
		return false;

	if (SSID != "" && IsHidden) {
		WiFi.ConnectAP(SSID, Password);
		return true;
	}

	if (SSID != "") // connect to specific WiFi SSID
		for (auto &WiFiScannedItem : WiFiScannedList) {
			ESP_LOGI("WiFiConnect", "%s %s", Converter::ToLower(WiFiScannedItem.getSSID()).c_str() , Converter::ToLower(SSID).c_str());
			if (Converter::ToLower(WiFiScannedItem.getSSID()) == Converter::ToLower(SSID)) {
				Log::Add(Log::Events::WiFi::STAConnecting);
				WiFi.ConnectAP(WiFiScannedItem.getSSID(), Password, WiFiScannedItem.getChannel());

				if (!DontUseCache) {
					uint32_t IP			= 0;
					uint32_t Gateway	= 0;
					uint32_t Netmask	= 0;

					for (auto &item : WiFiSettings)
						if (Converter::ToLower(item.SSID) == Converter::ToLower(SSID)) {
							IP 		= item.IP;
							Gateway	= item.Gateway;
							Netmask = item.Netmask;
							break;
						}

					if (IP != 0 && Gateway != 0 && Netmask !=0) {
						WiFi.SetIPInfo(IP, Gateway, Netmask);
						//!WiFi.AddDNSServer(inet_ntoa(Gateway));
					}
				}

				return true;
			}
		}

	if (SSID == "")
		for (auto &WiFiScannedItem : WiFiScannedList) {
			ESP_LOGI(tag, "WiFiScannedItem %s", WiFiScannedItem.getSSID().c_str());
				for (auto &item : WiFiSettings)
					if (item.SSID == WiFiScannedItem.getSSID()) {

						if (!DontUseCache && item.IP != 0 && item.Gateway != 0 && item.Netmask !=0) {
							ESP_LOGI("tag", "ip %d Gateway %d Netmask %d", item.IP, item.Gateway, item.Netmask);
							WiFi.SetIPInfo(item.IP, item.Gateway, item.Netmask);
							//!WiFi.AddDNSServer(inet_ntoa(item.Gateway));
						}

						Log::Add(Log::Events::WiFi::STAConnecting);
						WiFi.ConnectAP(WiFiScannedItem.getSSID(), item.Password, WiFiScannedItem.getChannel());
						return true;
					}
		}

	return false;
}

void Network_t::UpdateWiFiIPInfo(string SSID, esp_netif_ip_info_t Data) {
	if (Data.ip.addr != 0 && Data.gw.addr != 0 && Data.netmask.addr != 0)
		for (auto& Item : WiFiSettings )
			if (Item.SSID == SSID) {
				bool IsChanged = (Item.IP != Data.ip.addr || Item.Gateway != Data.gw.addr || Item.Netmask != Data.netmask.addr)
						? true : false;

				Item.IP 		= Data.ip.addr;
				Item.Gateway 	= Data.gw.addr;
				Item.Netmask	= Data.netmask.addr;

				if (IsChanged)
					SaveAccessPointsList();

				return;
			}
}


void Network_t::AddWiFiNetwork(string SSID, string Password) {
	if (SSID == "" || Password == "")
		return;

	bool IsExist = false;

	for (auto &item : WiFiSettings)
		if (item.SSID == SSID) {
			IsExist = true;
			item.Password = Password;
		}

	if (!IsExist) {
		if (WiFiSettings.size() >= Settings.WiFi.SavedAPCount)
			WiFiSettings.erase(WiFiSettings.begin());

		WiFiSettingsItem Item;
		Item.SSID 		= SSID;
		Item.Password 	= Password;

		WiFiSettings.push_back(Item);
	}

	SaveAccessPointsList();
}

bool Network_t::RemoveWiFiNetwork(string SSID) {
	if (SSID == "")
		return false;

	bool IsSuccess = false;

	for (auto it = WiFiSettings.begin(); it != WiFiSettings.end(); it++)
		if (Converter::ToLower((*it).SSID) == Converter::ToLower(SSID)) {
			WiFiSettings.erase(it--);
			IsSuccess = true;
		}

	SaveAccessPointsList();

	return IsSuccess;
}

void Network_t::LoadAccessPointsList() {
	NVS Memory(NVSNetworkArea);

	JSON JSONObject(Memory.GetString(NVSNetworkWiFiSettings));

	WiFiSettings.clear();
	for (auto& JSONItem : JSONObject.GetObjectsArray()) {
		WiFiSettingsItem Item;

		if (JSONItem.count("ssid")) 	Item.SSID 			= JSONItem["ssid"];
		if (JSONItem.count("password")) Item.Password 		= JSONItem["password"];
		if (JSONItem.count("channel")) 	Item.Channel 		= (uint8_t)Converter::ToUint16(JSONItem["channel"]);
		if (JSONItem.count("ip")) 		Item.IP				= Converter::ToUint32(JSONItem["ip"]);
		if (JSONItem.count("gateway")) 	Item.Gateway 		= Converter::ToUint32(JSONItem["gateway"]);
		if (JSONItem.count("netmask")) 	Item.Netmask 		= Converter::ToUint32(JSONItem["netmask"]);

		WiFiSettings.push_back(Item);
	}
}

void Network_t::SaveAccessPointsList() {
	vector<map<string,string>> Serialized = {};
	for (auto &Item : WiFiSettings) {
		Serialized.push_back({
			{ "ssid"	, Item.SSID 									},
			{ "password", Item.Password									},
			{ "channel" , Converter::ToString<uint8_t>(Item.Channel)	},
			{ "ip" 		, Converter::ToString<uint32_t>(Item.IP)		},
			{ "gateway" , Converter::ToString<uint32_t>(Item.Gateway)	},
			{ "netmask" , Converter::ToString<uint32_t>(Item.Netmask)	}
		});
	}

	JSON JSONObject;
	JSONObject.SetObjectsArray("", Serialized);

	NVS Memory(NVSNetworkArea);
	Memory.SetString(NVSNetworkWiFiSettings, JSONObject.ToString());
	Memory.Commit();
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

	if (!(JSONObject.GetItem("ID").empty()))      	Result.ID      			= Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("ID"));
	if (!(JSONObject.GetItem("IP").empty()))      	Result.IP      			= JSONObject.GetItem("IP");
	if (!(JSONObject.GetItem("TypeHex").empty())) 	Result.TypeHex 			= (uint8_t)strtol((JSONObject.GetItem("TypeHex")).c_str(), NULL, 16);

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
				WiFiSettingsVector.push_back(Item.SSID);

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

			if (URLParts[0] == "savedssid") {
				vector<string> WiFiSettingsVector = vector<string>();
				for (auto &Item : WiFiSettings)
					WiFiSettingsVector.push_back(Item.SSID);

				Result.Body = JSON::CreateStringFromVector(WiFiSettingsVector);
				Result.ContentType = WebServer_t::Response::TYPE::JSON;
			}

			if (URLParts[0] == "map") {
				vector<map<string,string>> NetworkMap = vector<map<string,string>>();
				for (auto& NetworkDevice: Devices)
					NetworkMap.push_back({
					{"Type"     , DeviceType_t::ToString(NetworkDevice.TypeHex)},
					{"ID"       , Converter::ToHexString(NetworkDevice.ID,8)},
					{"IP"       , NetworkDevice.IP},
					{"IsActive" , (NetworkDevice.IsActive == true) ? "1" : "0" }
				});

				JSON JSONObject;
				JSONObject.SetObjectsArray("", NetworkMap);

				Result.Body = JSONObject.ToString();
				Result.ContentType = WebServer_t::Response::TYPE::JSON;
			}

			if (URLParts[0] == "keepwifi") {
				KeepWiFiTimer = Settings.WiFi.KeepWiFiTime;

				Result.SetSuccess();
			}
		}

		if (URLParts.size() == 2) {
			if (URLParts[0] == "connect") {
				bool IsHidden = (Params.count("hidden") > 0);

				if (!WiFiConnect(URLParts[1], false, IsHidden)) {
					Result.ResponseCode = WebServer_t::Response::CODE::ERROR;
					Result.Body = "{\"success\" : \"false\"}";
					return;
				}
			}

			if (URLParts[0] == "remotecontrol") {
				if (URLParts[1] == "reconnect") {
					MQTT.Reconnect();
				}
			}
		}

		if (URLParts.size() == 3) {
			if (URLParts[0] == "remotecontrol" && URLParts[1] == "stop") {
				if (Converter::ToLower(MQTT.GetClientID()) == URLParts[2]) {
					MQTT.SetCredentials("","");
					//MQTT.Stop();
					Result.Body = "{\"success\" : \"true\"}";
					return;
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

			AddWiFiNetwork(Converter::StringURLDecode(Params["wifissid"]), Converter::StringURLDecode(Params["wifipassword"]));
			Result.SetSuccess();
			return;
		}
	}

	// DELETE запрос - удаление данных
	if (Type == QueryType::DELETE) {
		if (URLParts.size() == 2 && URLParts[0] == "savedssid")
		{
			vector<string> SSIDToDelete = Converter::StringToVector(URLParts[1], ",");

			bool IsSuccess = false;

			for (auto& SSIDItemToDelete : SSIDToDelete)
				if (Network.RemoveWiFiNetwork(SSIDItemToDelete))
					IsSuccess = true;

			if (SSIDToDelete.size() > 0 && IsSuccess)
				Result.SetSuccess();
			else
				Result.SetInvalid();
		}
	}
}

string Network_t::ModeToString() {
	return (WiFi_t::GetMode() == WIFI_MODE_AP_STR) ? "AP" : "Client";
}

string Network_t::IPToString() {
	esp_netif_ip_info_t IP = WiFi.GetIPInfo();
	return inet_ntoa(IP.ip);
}

vector<string> Network_t::GetSavedWiFiList() {
	vector<string> Result = {};

	for (auto &Item : WiFiSettings)
		Result.push_back(Item.SSID);

	return Result;
}

string Network_t::WiFiCurrentSSIDToString() {
	return WiFi_t::GetSSID();
}

/*
 *
 * NetworkSync class implementation
 *
 */

uint8_t		NetworkSync::SameQueryCount 		= 0;
uint16_t	NetworkSync::MaxStorageVersion 		= 0x0;
string		NetworkSync::SourceIP 				= "";
string		NetworkSync::Chunk 					= "";
uint16_t	NetworkSync::ToVersionUpgrade		= 0;

static string			SourceIP;

void NetworkSync::Start() {
	ESP_LOGE("MAP RECEIVED", "UDP Timer");

	int Index = -1;
	uint16_t CurrentVersion		= Storage.CurrentVersion();
	MaxStorageVersion = 0;

	for (int i = 0; i < Network.Devices.size(); i++)
		if (Network.Devices.at(i).StorageVersion > MaxStorageVersion) {
			MaxStorageVersion = Network.Devices.at(i).StorageVersion;
			Index = i;
		}

	if (Index == -1)
		return;

	if (MaxStorageVersion > CurrentVersion) {
		SourceIP = Network.Devices.at(Index).IP;
		SameQueryCount = 0;
		NetworkSync::StorageHistoryQuery();
	}

}

void NetworkSync::StorageHistoryQuery() {
	if (SameQueryCount >= 3)
		return;

	string QueryURL = "http://" + SourceIP + "/storage/history/upgrade?from=" + Converter::ToHexString((Storage.CurrentVersion() < 0x2000) ? 0 : Storage.CurrentVersion(), 4);
	ESP_LOGE("StorageHistoryQuery", "%s", QueryURL.c_str());

	ToVersionUpgrade 	= 0;
	HTTPClient::Query(QueryURL, 80, QueryType::GET, true, &ReadStorageStarted, &ReadStorageBody, &ReadStorageFinished, &StorageAborted);
}

void NetworkSync::ReadStorageStarted (char IP[]) {}

bool NetworkSync::ReadStorageBody (char Data[], int DataLen, char IP[]) {

	if (ToVersionUpgrade == 0)
		Chunk = string(Data,DataLen);
	else if (string(Data,DataLen) == ",") {
		Chunk = "";
		return true;
	}
	else
		Chunk += string(Data, DataLen);

	ESP_LOGE("CHUNK ", "%s", Chunk.c_str());

	if (ToVersionUpgrade == 0) {
		JSON JSONItem(Chunk + "]}");

		if (JSONItem.GetItems().count("to"))
			ToVersionUpgrade  = Converter::UintFromHexString<uint16_t>(JSONItem.GetItem("to"));

		Chunk = "";

		if (ToVersionUpgrade == 0)
			return false;
	}
	else if (Chunk.size() > 0)
	{
		if (Chunk.substr(Chunk.size() - 1) == ",")
			Chunk = Chunk.substr(0, Chunk.size() - 1);

		JSON JSONItem(Chunk);

		Storage_t::Item ItemToSave;

		if (!JSONItem.IsItemExists("id") || !JSONItem.IsItemExists("typeid") || !JSONItem.IsItemExists("data"))
			return true;

		ItemToSave.Header.MemoryID	= Converter::UintFromHexString<uint16_t>(JSONItem.GetItem("id"));
		ItemToSave.Header.TypeID	= Converter::UintFromHexString<uint8_t>(JSONItem.GetItem("typeid"));
		ItemToSave.SetData(JSONItem.GetItem("data"));

		if (ItemToSave.GetData().size() == 0 || ItemToSave.Header.TypeID == Settings.Memory.Empty8Bit)
			return true;

		Storage.Write(ItemToSave);
		ESP_LOGE("Storage Sync", "Item with ID %d writed", ItemToSave.Header.MemoryID);

		Chunk = "";
	}

	return true;
}

void NetworkSync::ReadStorageFinished(char IP[]) {
	SameQueryCount = 0;
	Chunk = "";

	if (ToVersionUpgrade == 0)
		return;

	Storage.Commit(ToVersionUpgrade);
	ESP_LOGE("Storage Sync", "Commited version %d", ToVersionUpgrade);

	if (ToVersionUpgrade < MaxStorageVersion)
		StorageHistoryQuery();
}

void NetworkSync::StorageAborted(char IP[]) {
	SameQueryCount++;
	Chunk = "";
	StorageHistoryQuery();

	ESP_LOGE("Network_t::StorageAborted", "");

}

