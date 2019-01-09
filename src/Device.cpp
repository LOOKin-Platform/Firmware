/*
 *    Device.cpp
 *    Class to handle API /Device
 *
 */
#include "Globals.h"
#include "Device.h"

static char tag[] = "Device_t";

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

	switch (Type.Hex) {
		case Settings.Devices.Plug	: PowerModeVoltage = +220; break;
		case Settings.Devices.Remote: PowerModeVoltage = +3; break;
	}
}

void Device_t::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
	if (Type == QueryType::GET) {
		// Запрос JSON со всеми параметрами
		if (URLParts.size() == 0) {
			JSON JSONObject;

			JSONObject.SetItems(vector<pair<string,string>> ({
				make_pair("Type"		, TypeToString()),
						make_pair("Status"			, StatusToString()),
						make_pair("ID"				, IDToString()),
						make_pair("Name"			, NameToString()),
						make_pair("Time"			, Time::UnixtimeString()),
						make_pair("Timezone"		, Time::TimezoneStr()),
						make_pair("PowerMode"		, PowerModeToString()),
						make_pair("CurrentVoltage"	, CurrentVoltageToString()),
						make_pair("Firmware"		, FirmwareVersionToString()),
						make_pair("Temperature"		, TemperatureToString())
			}));

			Result.Body = JSONObject.ToString();
		}

		// Запрос конкретного параметра
		if (URLParts.size() == 1) {
			if (URLParts[0] == "type")			Result.Body = TypeToString();
			if (URLParts[0] == "status")		Result.Body = StatusToString();
			if (URLParts[0] == "id")			Result.Body = IDToString();
			if (URLParts[0] == "name")			Result.Body = NameToString();
			if (URLParts[0] == "time")			Result.Body = Time::UnixtimeString();
			if (URLParts[0] == "timezone")		Result.Body = Time::TimezoneStr();
			if (URLParts[0] == "powermode")		Result.Body = PowerModeToString();
			if (URLParts[0] == "currentvoltage")Result.Body = CurrentVoltageToString();
			if (URLParts[0] == "firmware")		Result.Body = FirmwareVersionToString();
			if (URLParts[0] == "temperature")	Result.Body = TemperatureToString();

			Result.ContentType = WebServer_t::Response::TYPE::PLAIN;
		}
	}

	// обработка POST запроса - сохранение и изменение данных
	if (Type == QueryType::POST) {
		if (URLParts.size() == 0) {
			bool isNameSet            = POSTName(Params);
			bool isTimeSet            = POSTTime(Params);
			bool isTimezoneSet        = POSTTimezone(Params);
			bool isFirmwareVersionSet = POSTFirmwareVersion(Params, Result);

			if ((isNameSet || isTimeSet || isTimezoneSet || isFirmwareVersionSet) && Result.Body == "")
				Result.Body = "{\"success\" : \"true\"}";
		}

		if (URLParts.size() == 2) {
			if (URLParts[0] == "firmware") {
				Result.Body = "{\"success\" : \"true\"}";

				if (URLParts[1] == "start")
					OTA::ReadStarted();

				if (URLParts[1] == "write") {
					if (Params.count("data") == 0) {
						Result.ResponseCode = WebServer_t::Response::INVALID;
						Result.Body = "{\"success\" : \"false\" , \"Error\": \"Writing data failed\"}";
						return;
					}

					string HexData = Params["data"];
					uint8_t *BinaryData = new uint8_t[HexData.length() / 2];

					for (int i=0; i < HexData.length(); i=i+2)
						BinaryData[i/2] = Converter::UintFromHexString<uint8_t>(HexData.substr(i, 2));

					if (!OTA::ReadBody(reinterpret_cast<char*>(BinaryData), HexData.length() / 2)) {
						Result.ResponseCode = WebServer_t::Response::INVALID;
						Result.Body = "{\"success\" : \"false\" , \"Error\": \"Writing data failed\"}";
					}

					delete BinaryData;
				}

				if (URLParts[1] == "finish")
					OTA::ReadFinished();
			}
		}
	}
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

uint8_t Device_t::GetTypeFromNVS() {
	NVS Memory(NVSDeviceArea);
	return Memory.GetInt8Bit(NVSDeviceType);
}

void Device_t::SetTypeToNVS(uint8_t Type) {
	NVS Memory(NVSDeviceArea);
	Memory.SetInt8Bit(NVSDeviceType, Type);
	Memory.Commit();
}

uint32_t Device_t::GetIDFromNVS() {
	NVS Memory(NVSDeviceArea);
	return Memory.GetUInt32Bit(NVSDeviceID);
}

void Device_t::SetIDToNVS(uint32_t ID) {
	NVS Memory(NVSDeviceArea);
	Memory.SetUInt32Bit(NVSDeviceID, ID);
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

bool Device_t::POSTFirmwareVersion(map<string,string> Params, WebServer_t::Response& Response) {
	if (Params.count("firmware") == 0)
		return false;

	if (Converter::ToFloat(Settings.FirmwareVersion) > Converter::ToFloat(Params["firmware"])) {
		Response.ResponseCode = WebServer_t::Response::CODE::OK;
		Response.Body = "{\"success\" : \"false\" , \"Message\": \"Attempt to update to the old version\"}";
		return false;
	}

	if (Status == DeviceStatus::UPDATING) {
		Response.ResponseCode = WebServer_t::Response::CODE::ERROR;
		Response.Body = "{\"success\" : \"false\" , \"Error\": \"The update process has already been started\"}";
		return false;
	}

	if (WiFi_t::GetMode() != WIFI_MODE_STA_STR) {
		Response.ResponseCode = WebServer_t::Response::CODE::ERROR;
		Response.Body = "{\"success\" : \"false\" , \"Error\": \"Device is not connected to the Internet\"}";
		return false;
	}

	string UpdateFilename = "firmware.bin";

	if (Params.count("filename") > 0)
		UpdateFilename = Params["filename"];

	OTA::Update(Settings.OTA.UrlPrefix + Params["firmware"] + "/" + UpdateFilename);
	Device.Status = DeviceStatus::UPDATING;

	Response.ResponseCode = WebServer_t::Response::CODE::OK;
	Response.Body = "{\"success\" : \"true\" , \"Message\": \"Firmware update started\"}";

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
	return FirmwareVersion;
}

string Device_t::TemperatureToString() {
	return Converter::ToString(Temperature);
}

string Device_t::CurrentVoltageToString() {
	return Converter::ToString(CurrentVoltage);
}
