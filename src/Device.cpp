/*
 *    Device.cpp
 *    Class to handle API /Device
 *
 */
#include "Globals.h"
#include "Device.h"
#include "HomeKit.h"
#include "BootAndRestore.h"

httpd_req_t	*Device_t::CachedRequest 	= NULL;

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
		case Settings.Devices.Duo:
		case Settings.Devices.Plug:
			PowerModeVoltage = +220;
			break;

		case Settings.Devices.Remote:
			PowerModeVoltage 	= +5;
			SensorMode 			= GetSensorModeFromNVS();
			break;
	}

	PowerManagement::SetPMType(GetEcoFromNVS(), (PowerMode == DevicePowerMode::CONST));
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
			if (Query.CheckURLPart("type"			, 1))	Result.Body = TypeToString();
			if (Query.CheckURLPart("mrdc"			, 1))	Result.Body = MRDCToString();
			if (Query.CheckURLPart("status"			, 1))	Result.Body = StatusToString();
			if (Query.CheckURLPart("id"				, 1))	Result.Body = IDToString();
			if (Query.CheckURLPart("name"			, 1))	Result.Body = NameToString();
			if (Query.CheckURLPart("time"			, 1))	Result.Body = Time::UnixtimeString();
			if (Query.CheckURLPart("timezone"		, 1))	Result.Body = Time::TimezoneStr();
			if (Query.CheckURLPart("powermode"		, 1))	Result.Body = PowerModeToString();
			if (Query.CheckURLPart("currentvoltage"	, 1))	Result.Body = CurrentVoltageToString();
			if (Query.CheckURLPart("firmware"		, 1))	Result.Body = FirmwareVersionToString();
			if (Query.CheckURLPart("temperature"	, 1))	Result.Body = TemperatureToString();
			if (Query.CheckURLPart("homekit"		, 1))	Result.Body = HomeKitToString();
			if (Query.CheckURLPart("ecomode"		, 1))	Result.Body = EcoToString();

			if ((Query.CheckURLPart("sensormode", 1)) && Device.Type.Hex == Settings.Devices.Remote)
				Result.Body = SensorModeToString();

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
		map<string,string> Params = JSON(Query.GetBody()).GetItems();

		if (Query.GetURLPartsCount() == 1) {
			bool isNameSet            	= POSTName(Params);
			bool isTimeSet            	= POSTTime(Params);
			bool isTimezoneSet        	= POSTTimezone(Params);
			bool isEcoSet				= POSTEco(Params);
			bool isFirmwareVersionSet 	= POSTFirmwareVersion(Params, Result, Query.GetRequest(), Query.Transport);
			bool isSensorModeSet		= POSTSensorMode(Params, Result);

			if ((isNameSet || isTimeSet || isTimezoneSet || isFirmwareVersionSet || isSensorModeSet || isEcoSet) && Result.Body == "")
				Result.Body = "{\"success\" : \"true\"}";
		}

		if (Query.GetURLPartsCount() == 2) {
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
}

JSON Device_t::RootInfo() {
	JSON JSONObject;

	JSONObject.SetItems(vector<pair<string,string>> ({
		make_pair("Type"			, TypeToString()),
		make_pair("MRDC"			, MRDCToString()),
		make_pair("Status"			, StatusToString()),
		make_pair("ID"				, IDToString()),
		make_pair("Name"			, NameToString()),
		make_pair("Time"			, Time::UnixtimeString()),
		make_pair("Timezone"		, Time::TimezoneStr()),
		make_pair("PowerMode"		, PowerModeToString()),
		make_pair("CurrentVoltage"	, CurrentVoltageToString()),
		make_pair("Firmware"		, FirmwareVersionToString()),
		make_pair("Temperature"		, TemperatureToString()),
		make_pair("HomeKit"			, HomeKitToString()),
		make_pair("EcoMode"			, EcoToString())
	}));

	if (Device.Type.Hex == Settings.Devices.Remote)
		JSONObject.SetItem("SensorMode", SensorModeToString());

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

bool Device_t::GetEcoFromNVS() {
	NVS Memory(NVSDeviceArea);
	return (Memory.GetInt8Bit(NVSDeviceEco) == 1) ? true : false;
}

void Device_t::SetEcoToNVS(bool Eco) {
	NVS Memory(NVSDeviceArea);
	Memory.SetInt8Bit(NVSDeviceEco, (Eco) ? 1 : 0);
	Memory.Commit();
}

void Device_t::SetIDToNVS(uint32_t ID) {
	NVS Memory(NVSDeviceArea);
	Memory.SetUInt32Bit(NVSDeviceID, ID);
	Memory.Commit();
}

bool Device_t::GetSensorModeFromNVS() {
	NVS Memory(NVSDeviceArea);
	return (Memory.GetString(NVSDeviceSensorMode) == "1") ? true : false;
}

void Device_t::SetSensorModeToNVS(bool SensorMode) {
	NVS Memory(NVSDeviceArea);
	Memory.SetString(NVSDeviceSensorMode, (SensorMode) ? "1" : "0");
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


static WebServer_t::QueryTransportType PostFirmwareTransportType = WebServer_t::QueryTransportType::WebServer;

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

	Device.Status = DeviceStatus::UPDATING;

	PostFirmwareTransportType = TransportType;

	OTA::Update(OTAUrl, &OTACallbackSuccessfulStarted, &OTACallbackFileDoesntExist);

	return true;
}

void Device_t::OTACallbackSuccessfulStarted() {
	Device.Status = DeviceStatus::UPDATING;

	WebServer_t::Response Response;
	Response.Body = "{\"success\" : \"true\" , \"Message\": \"Firmware update will be started if file exists. Web server temporarily stopped.\"}";
	Response.ResponseCode = WebServer_t::Response::CODE::OK;

	if (PostFirmwareTransportType == WebServer_t::QueryTransportType::WebServer)
		WebServer_t::SendHTTPData(Response, Device_t::CachedRequest);
}

void Device_t::OTACallbackFileDoesntExist() {
	Device.Status = DeviceStatus::RUNNING;

	WebServer_t::Response Response;
	Response.Body = "{ \"success\": \"false\", \"Message\": \"Firmware update failed. No such file.\" }";
	Response.ResponseCode = WebServer_t::Response::CODE::ERROR;

	WebServer_t::SendHTTPData(Response, Device_t::CachedRequest);
}

bool Device_t::POSTSensorMode(map<string,string> Params, WebServer_t::Response& Response)
{
	if (Params.count("sensormode") > 0) {
		if (Params["sensormode"] == "1" || Converter::ToLower(Params["sensormode"]) == "true")
			SensorMode = true;

		if (Params["sensormode"] == "0" || Converter::ToLower(Params["sensormode"]) == "false")
			SensorMode = false;

		SetSensorModeToNVS(SensorMode);

		return true;
	}

	return false;
}

bool Device_t::POSTEco(map<string,string> Params) {
	if (Params.count("ecomode") > 0) {
		bool EcoOn = Converter::ToLower(Params["ecomode"]) == "on" ? true : false;
		SetEcoToNVS(EcoOn);

		PowerManagement::SetPMType(EcoOn, (Device.PowerMode == CONST));

		Log::Add((EcoOn) ? Log::Events::System::PowerManageOn : Log::Events::System::PowerManageOff);

		return true;
	}

	return false;
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

string Device_t::SensorModeToString() {
	return (SensorMode) ? "1" : "0";
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

string Device_t::HomeKitToString() {
	if (!HomeKit::IsEnabledForDevice()) return "0";
	return (HomeKit::IsExperimentalMode()) ? "2" : "1";
}

string Device_t::EcoToString() {
	return GetEcoFromNVS() ? "on" : "off";
}

