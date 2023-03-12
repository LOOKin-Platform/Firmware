/*
 *    Device.cpp
 *    Class to handle API /Device
 *
 */
#include "Globals.h"
#include "Device.h"
#include "Matter.h"
#include "BootAndRestore.h"
#include "NimBLEDevice.h"

httpd_req_t		*Device_t::CachedRequest 	= NULL;
string 			Device_t::FirmwareURLForOTA = "";
TaskHandle_t	Device_t::OTATaskHandler 	= NULL;

DeviceType_t::DeviceType_t(string TypeStr) {
	for(auto const &TypeItem : Settings.Devices.Literaly) {
		if (Converter::ToLower(TypeItem.second) == Converter::ToLower(TypeStr))
			Hex = TypeItem.first;
	}
}

DeviceType_t::DeviceType_t(uint8_t Type) {
	Hex = Type;
}

bool DeviceType_t::IsBattery() {
	if (Hex < 0x80)
		return false;

	if (Hex == Settings.Devices.Remote)
		return (Device.PowerMode == DevicePowerMode::BATTERY);

	return true;
}

string DeviceType_t::ToString() {
	return ToString(Hex);
}

string DeviceType_t::ToHexString() {
	return Converter::ToHexString(Hex,2);
}

string DeviceType_t::ToString(uint8_t Hex) {
	string Result;

	if (Settings.Devices.Literaly.count(Hex) > 0)
		Result = Settings.Devices.Literaly[Hex];

	return Result;
}

Device_t::Device_t() { }

void Device_t::Init() {
	Type = DeviceType_t(Settings.eFuse.Type);
	PowerMode = (Type.IsBattery()) ? DevicePowerMode::BATTERY : DevicePowerMode::CONST;

	Capabilities.Raw = 0;

	switch (Type.Hex) 
	{
		case Settings.Devices.Duo:
		case Settings.Devices.Plug:
			PowerModeVoltage 	= +220;
			break;

		case Settings.Devices.Remote:
			PowerModeVoltage 	= +5;
			break;

		case Settings.Devices.WindowOpener:
			PowerModeVoltage 	= +5;
			break;
	}
	
	LoadCapabilityFlagsFromNVS();
	PowerManagement::SetPMType(IsEcoModeEnabled(), (PowerMode == DevicePowerMode::CONST));

	IsAutoUpdate = GetAutoUpdateFromNVS();
}

void Device_t::HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) {
	if (Query.Type == QueryType::GET) {
		// Запрос JSON со всеми параметрами
		if (Query.GetURLPartsCount() == 1) {
			Result.Body = RootInfo().ToString();
			return;
		}

		// Запрос конкретного параметра
		if (Query.GetURLPartsCount() == 2) {
			if (Query.CheckURLPart("type"				, 1))	Result.Body = TypeToString();
			if (Query.CheckURLPart("mrdc"				, 1))	Result.Body = MRDCToString();
			if (Query.CheckURLPart("status"				, 1))	Result.Body = StatusToString();
			if (Query.CheckURLPart("id"					, 1))	Result.Body = IDToString();
			if (Query.CheckURLPart("name"				, 1))	Result.Body = NameToString();
			if (Query.CheckURLPart("time"				, 1))	Result.Body = Time::UnixtimeString();
			if (Query.CheckURLPart("timezone"			, 1))	Result.Body = Time::TimezoneStr();
			if (Query.CheckURLPart("powermode"			, 1))	Result.Body = PowerModeToString();
			if (Query.CheckURLPart("currentvoltage"		, 1))	Result.Body = CurrentVoltageToString();
			if (Query.CheckURLPart("firmware"			, 1))	Result.Body = FirmwareVersionToString();
			if (Query.CheckURLPart("temperature"		, 1))	Result.Body = TemperatureToString();
			if (Query.CheckURLPart(JSONFieldCapabilities, 1))	Result.Body = Converter::ToHexString(Capabilities.Raw,4);

			Result.ContentType = WebServer_t::Response::TYPE::PLAIN;
		}

		if (Query.GetURLPartsCount() == 3){
			if ((Query.CheckURLPart("ota", 1)) && (Query.CheckURLPart("rollback", 2))) {
				if (OTA::Rollback()) {
					Result.Body = "{\"success\" : \"true\" , \"Description\": \"Device will be restarted shortly\"}";
				}
				else
					Result.Body = "{\"error\": \"Can't find partition to rollback\"}";

				return;
			}
		}
	}

	// обработка POST запроса - сохранение и изменение данных
	if (Query.Type == QueryType::POST) {
		JSON JSONObject = JSON(Query.GetBody());
		map<string,string> Params = JSONObject.GetItems();

		if (Query.GetURLPartsCount() == 1) {
			bool isNameSet            	= POSTName(Params);
			bool isTimeSet            	= POSTTime(Params);
			bool isTimezoneSet        	= POSTTimezone(Params);
			bool isFirmwareVersionSet 	= POSTFirmwareVersion(Params, Result, Query.GetRequest(), Query.Transport);
			bool IsAutoUpdateSet		= POSTIsAutoUpdate(Query);

			if ((isNameSet || isTimeSet || isTimezoneSet || isFirmwareVersionSet || IsAutoUpdateSet) && Result.Body == "")
				Result.Body = "{\"success\" : \"true\"}";
		}

		if (Query.GetURLPartsCount() == 2) {
			if (Query.CheckURLPart(JSONFieldCapabilities, 1)) {
				bool IsSucceed = POSTCapabilites(Query.GetBody());

				if (IsSucceed)
					Result.SetSuccess();
				else 
					Result.SetFail("Incorrect capability format");

				return;
			}

			if (Query.CheckURLPart("factory-reset", 1)) {
				Result.SetSuccess();
				BootAndRestore::HardReset(true);
				return;
			}

			if (Query.CheckURLPart("reboot", 1)) {
				Result.SetSuccess();
				BootAndRestore::Reboot(true);
				return;
			}

			if (Query.CheckURLPart("rollback", 1)) {
				Result.SetSuccess();
				OTA::Rollback();
				return;
			}
		}


		if (Query.GetURLPartsCount() == 3) {
			if (Query.CheckURLPart("firmware", 1)) {
				Result.Body = "{\"success\" : \"true\"}";

				if (Query.CheckURLPart("start", 2))
					OTA::ReadStarted("");

				if (Query.CheckURLPart("write", 2)) {
					if (Params.count("data") == 0) {
						Result.ResponseCode = WebServer_t::Response::INVALID;
						Result.Body = "{\"success\" : \"false\" , \"Error\": \"Writing data failed\"}";
						return;
					}

					string HexData = Params["data"];
					uint8_t *BinaryData = new uint8_t[HexData.length() / 2];

					for (int i=0; i < HexData.length(); i=i+2)
						BinaryData[i/2] = Converter::UintFromHexString<uint8_t>(HexData.substr(i, 2));

					if (!OTA::ReadBody(reinterpret_cast<char*>(BinaryData), HexData.length() / 2, "")) {
						Result.ResponseCode = WebServer_t::Response::INVALID;
						Result.Body = "{\"success\" : \"false\" , \"Error\": \"Writing data failed\"}";
					}

					delete BinaryData;
				}

				if (Query.CheckURLPart("finish", 2))
					OTA::ReadFinished("");
			}
		}
	}

	// обработка PUT запроса - изменение данных
	if (Query.Type == QueryType::PUT) {
		JSON JSONObject = JSON(Query.GetBody());

		if (Query.GetURLPartsCount() == 2) { // Operation to logical end for capabilities
			if (Query.CheckURLPart(JSONFieldCapabilities, 1)) {
				bool IsSucceed = PUTCapabilities(Query.GetBody());

				if (IsSucceed)
					Result.SetSuccess();
				else 
					Result.SetFail("Incorrect capability format");

				return;
			}
		}
	}

	if (Query.CheckURLMask("POST /device/efuse")) {
		JSON JSONItem(Query.GetBody());

		if (!JSONItem.IsItemExists("token")) 						{ Result.SetInvalid(); return; }
		if (JSONItem.GetItem("token") != "APGqjT3KPNeSHdY2ufCI") 	{ Result.SetInvalid(); return; }
		if (JSONItem.GetKeys().size() == 1) 						{ Result.SetInvalid(); return; }

		NVS Memory(NVSDeviceArea);
		if (JSONItem.IsItemExists("ID") && JSONItem.IsItemString("ID"))
			Memory.SetUInt32Bit(NVSDeviceID, (uint32_t)Converter::UintFromHexString<uint32_t>(JSONItem.GetItem("ID")));

		if (JSONItem.IsItemExists("Type") && JSONItem.IsItemNumber("Type"))
			Memory.SetUInt16Bit(NVSDeviceType, JSONItem.GetIntItem("Type"));

		if (JSONItem.IsItemExists("Model") && JSONItem.IsItemNumber("Model"))
			Memory.SetInt8Bit(NVSDeviceModel, JSONItem.GetIntItem("Model"));

		if (JSONItem.IsItemExists("Revision") && JSONItem.IsItemNumber("Revision"))
			Memory.SetUInt16Bit(NVSDeviceRevision, JSONItem.GetIntItem("Revision"));

		Memory.Commit();

		Result.SetSuccess();
		return;
	}
}

JSON Device_t::RootInfo() {
	JSON JSONObject;

	JSONObject.SetItems(vector<pair<string,string>> ({
		make_pair("Type"				, TypeToString()),
		make_pair("MRDC"				, MRDCToString()),
		make_pair("Status"				, StatusToString()),
		make_pair("ID"					, IDToString()),
		make_pair("Name"				, NameToString()),
		make_pair("Time"				, Time::UnixtimeString()),
		make_pair("Timezone"			, Time::TimezoneStr()),
		make_pair("PowerMode"			, PowerModeToString()),
		make_pair("CurrentVoltage"		, CurrentVoltageToString()),
		make_pair("Firmware"			, FirmwareVersionToString()),
		make_pair("Temperature"			, TemperatureToString()),
		make_pair("BLEStatus"			, Converter::ToHexString(Wireless.GetBLEStatus(),2)),
		make_pair(JSONFieldCapabilities	, Converter::ToHexString(Capabilities.Raw,4))
	}));

	JSONObject.SetBoolItem("IsAutoUpdate", IsAutoUpdate);

	return JSONObject;
}

string Device_t::GetName() {
	NVS Memory(NVSDeviceArea);
	return Memory.GetString(NVSDeviceName);
}

void Device_t::SetName(string Name) {
	NVS Memory(NVSDeviceArea);
	Memory.SetString(NVSDeviceName, Name);
	Memory.Commit();
}

bool Device_t::GetAutoUpdateFromNVS() {
	NVS Memory(NVSDeviceArea);

	uint8_t Result = Memory.GetInt8Bit(NVSDeviceAutoUpdate);

	if (Result == 0) // not inited, return true
		Result = 2;

	return (Result == 2) ? true : false;
}

void Device_t::SetAutoUpdateToNVS(bool AutoUpdateNew) {
	NVS Memory(NVSDeviceArea);
	Memory.SetInt8Bit(NVSDeviceAutoUpdate, (AutoUpdateNew) ? 2 : 1); // 2 for true, 1 for false
	Memory.Commit();

	IsAutoUpdate = AutoUpdateNew;
}

void Device_t::LoadCapabilityFlagsFromNVS() {
	NVS Memory(NVSDeviceArea);
	Capabilities.Raw = Memory.GetUInt16Bit(NVSDeviceCapabilities);

	if (Capabilities.Raw == 0x0) // Set default value
		Capabilities.IsMatterEnabled = true;
}

void Device_t::SetCapabilityFlagsToNVS() {
	NVS Memory(NVSDeviceArea);
	Memory.SetUInt16Bit(NVSDeviceCapabilities, Capabilities.Raw);
	Memory.Commit();
}

// Генерация ID на основе MAC-адреса чипа
uint32_t Device_t::GenerateID() {
	uint8_t mac[6];
	esp_efuse_mac_get_default(mac);

	stringstream MacString;
	MacString << std::dec << (int) mac[0] << (int) mac[1] << (int) mac[2] << (int) mac[3] << (int) mac[4] << (int) mac[5];

	hash<std::string> hash_fn;
	size_t str_hash = hash_fn(MacString.str());

	stringstream Hash;
	Hash << std::uppercase << std::hex << (int)str_hash;

	return Converter::UintFromHexString<uint32_t>(Hash.str());
}

bool Device_t::POSTName(map<string,string> Params) {
	if (Params.count("name") > 0) {
		SetName(Params["name"]);
		return true;
	}

	return false;
}

bool Device_t::POSTTime(map<string,string> Params) {
	if (Params.count("time") > 0) {
		Time::SetTime(Params["time"]);
		return true;
	}

	return false;
}

bool Device_t::POSTTimezone(map<string,string> Params) {
	if (Params.count("timezone") > 0) {
		Time::SetTimezone(Params["timezone"]);
		return true;
	}

	return false;
}

bool Device_t::POSTFirmwareVersion(map<string,string> Params, WebServer_t::Response& Response, httpd_req_t *Request, WebServer_t::QueryTransportType TransportType) {
	if (Params.count("firmware") == 0)
		return false;

	if (Converter::ToLower(Params["firmware"]).find("http") != 0)
		if (Converter::ToFloat(Settings.Firmware.ToString()) > Converter::ToFloat(Params["firmware"])) {
			Response.ResponseCode = WebServer_t::Response::CODE::OK;
			Response.Body = "{\"success\" : \"false\" , \"Message\": \"Attempt to update to the old version\"}";
			return false;
		}

	if (Status == DeviceStatus::UPDATING) {
		Response.ResponseCode = WebServer_t::Response::CODE::ERROR;
		Response.Body = "{\"success\" : \"false\" , \"Error\": \"The update process has already been started\"}";
		return false;
	}

	string OTAUrl = Params["firmware"];

	if (Converter::ToLower(Params["firmware"]).find("http") != 0) {
		string UpdateFilename = "firmware.bin";
/*
#if (Settings.DeviceGeneration == 2)
		string UpdateFilename = "firmware_4mb.bin";
#else
		string UpdateFilename = "firmware.bin";
#endif
*/
		if (Params["filename"].size() > 0)
			UpdateFilename = Params["filename"];

		OTAUrl = Settings.OTA.APIUrl + Params["firmware"] + "/" + UpdateFilename;
	}

	CachedRequest = Request;

	OTAStart(OTAUrl, TransportType);

	return true;
}

void Device_t::OTAStart(string FirmwareURL, WebServer_t::QueryTransportType TransportType) {
	FirmwareURLForOTA = FirmwareURL;

	if (FirmwareURLForOTA.length() == 0)
		return;

	if (TransportType == WebServer_t::QueryTransportType::WebServer) {
		WebServer_t::Response Response;
		Response.Body = "{\"success\" : \"true\" , \"Message\": \"Firmware update will be started if file exists. Web server temporarily stopped.\"}";
		Response.ResponseCode = WebServer_t::Response::CODE::OK;

		WebServer_t::SendHTTPData(Response, Device_t::CachedRequest, false);
	}
	else if (TransportType == WebServer_t::QueryTransportType::MQTT) {
		RemoteControl.SendMessage("{\"success\" : \"true\" , \"Message\": \"Firmware update will be started if file exists. Web server temporarily stopped.\"}");
	}

	LocalMQTT.Stop();
	Matter::Stop();

	if (NimBLEDevice::getAdvertising()->isAdvertising()) {
		NimBLEDevice::stopAdvertising();
		FreeRTOS::Sleep(1000);
		NimBLEDevice::deinit(false);
	}

	FreeRTOS::Sleep(2000);

	if (OTATaskHandler == NULL) {
		OTATaskHandler = FreeRTOS::StartTask(Device_t::ExecuteOTATask, "ExecuteOTATask", NULL, 10240, 7);
	}
}

void Device_t::ExecuteOTATask(void*) {
	ESP_LOGE("OTA UPDATE STARTED", "");

	RemoteControl.Stop();

	ESP_LOGI("Pooling","RAM left %lu", esp_get_free_heap_size());

	OTA::Update(FirmwareURLForOTA, OTAStartedCallback, OTAFailedCallback);

	OTATaskHandler = NULL;
    FreeRTOS::DeleteTask();
}

void Device_t::OTAStartedCallback() {
	Device.Status = DeviceStatus::UPDATING;
	Log::Add(Log::Events::OTAStarted);
}

void Device_t::OTAFailedCallback() {
	Log::Add(Log::Events::OTAFailed);

	Device.Status = DeviceStatus::RUNNING;

	RemoteControl.Start();
	LocalMQTT.Start();
	Matter::Start();
}

bool Device_t::POSTIsAutoUpdate(Query_t& Query) {
	JSON JSONObject(Query.GetBody());

	if (JSONObject.IsItemExists("isautoupdate"))
	{
		bool NewAutoUpdateValue = JSONObject.GetBoolItem("isautoupdate");

		if (NewAutoUpdateValue != IsAutoUpdate)
			SetAutoUpdateToNVS(NewAutoUpdateValue);

		return true;
	}

	return false;
}

bool Device_t::POSTCapabilites(const char * Data) {
	if (strlen(Data) < 3)
		return false;

	string CapabilityRawString(Data);

	uint16_t CapabilitiesToSet = 0;

	if (CapabilityRawString.substr(0,2) == "0x" || CapabilityRawString.substr(0,2) == "0b") {
		string Prefix = CapabilityRawString.substr(0,2);
		CapabilityRawString = CapabilityRawString.substr(2);

		if (Prefix == "0x") {
			CapabilitiesToSet = Converter::UintFromHexString<uint16_t>(CapabilityRawString);	
		}
		else if (Prefix == "0b") {
			while (CapabilityRawString.size() < 16)
				CapabilityRawString = "0" + CapabilityRawString;

			CapabilitiesToSet = std::strtol(CapabilityRawString.c_str(), nullptr, 2);
		}

		return SetCapabilities(CapabilitiesToSet);
	}

	return false;
}

bool Device_t::PUTCapabilities(const char * Data) {
	if (strlen(Data) < 3)
		return false;

	string CapabilityRawString(Data);

	uint16_t CapabilitiesToSet = 0;

	if (CapabilityRawString.substr(0,2) == "0x" || CapabilityRawString.substr(0,2) == "0b") {
		string Prefix = CapabilityRawString.substr(0,2);
		CapabilityRawString = CapabilityRawString.substr(2);

		uint16_t Divider = 0;

		if (Prefix == "0x") {
			Divider = Converter::UintFromHexString<uint16_t>(CapabilityRawString);	
		}
		else if (Prefix == "0b") {
			while (CapabilityRawString.size() < 16)
				CapabilityRawString = "0" + CapabilityRawString;

			Divider = std::strtol(CapabilityRawString.c_str(), nullptr, 2);
		}

		CapabilitiesToSet = Capabilities.Raw & Divider;
		return SetCapabilities(CapabilitiesToSet);
	}

	return false;
}

bool Device_t::SetCapabilities(uint16_t NewCapabilities) {

	Capabilities.Raw = NewCapabilities;

	if (Capabilities.IsMatterEnabled && 
		(Capabilities.IsRemoteControlEnabled || Capabilities.IsLocalMQTTEnabled)) 
		{
			Capabilities.IsRemoteControlEnabled 	= false;
			Capabilities.IsLocalMQTTEnabled			= false;
		}

	SetCapabilityFlagsToNVS();

	return true;
}



string Device_t::TypeToString() {
	return Type.ToString();
}

string Device_t::StatusToString() {
	switch (Status) {
		case DeviceStatus::RUNNING  : return "Running"	; break;
		case DeviceStatus::UPDATING : return "Updating"	; break;
		default: return "";
	}
}

string Device_t::IDToString() {
	return (Settings.eFuse.DeviceID > 0) ? Converter::ToHexString(Settings.eFuse.DeviceID, 8) : "";
}

string Device_t::NameToString() {
	return GetName();
}

string Device_t::PowerModeToString() {
	stringstream s;
	s << std::dec << +PowerModeVoltage;

	return (PowerMode == DevicePowerMode::BATTERY) ? "Battery": s.str() + "v";
}

string Device_t::FirmwareVersionToString() {
	return Settings.Firmware.ToString();
}

string Device_t::TemperatureToString() {
	return Converter::ToString(Temperature);
}

string Device_t::CurrentVoltageToString() {
	return Converter::ToString(CurrentVoltage);
}

string Device_t::ModelToString() {
	return Converter::ToHexString(((Settings.eFuse.Model == 0) ? 1 : Settings.eFuse.Model), 2);
}

string Device_t::MRDCToString() {
	return ModelToString()
			+ Converter::ToHexString((Settings.eFuse.Revision),4)
			+ Converter::ToHexString(Settings.eFuse.Produced.Destination,2)
			+ Converter::ToHexString(Settings.eFuse.Produced.Factory,2)
			+ Converter::ToHexString(Settings.eFuse.Produced.Day, 2)
			+ Converter::ToHexString(Settings.eFuse.Produced.Month, 1)
			+ Converter::ToHexString(Settings.eFuse.Produced.Year, 3);
}