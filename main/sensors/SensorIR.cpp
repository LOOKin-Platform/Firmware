/*
*    SensorIR.cpp
*    SensorIR_t implementation
*
*/

#include "SensorIR.h"

#include <IRLib.h>
#include <ISR.h>
#include "HTTPClient.h"

#include "Data.h"
#include "DataRemote.h"
#include "Storage.h"
#include "Wireless.h"
#include "Automation.h"
#include "LocalMQTT.h"

#include "esp_timer.h"
#include "RMT.h"

extern DataEndpoint_t 	*Data;
extern Storage_t 		Storage;
extern Wireless_t 		Wireless;
extern Automation_t		Automation;

SensorIR_t::SensorIR_t() {
	if (GetIsInited()) return;

	ID          = SensorIRID;
	Name        = "IR";
	EventCodes  = { 0x00, 0x01, 0xEE, 0xFF };

	if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0 && Settings.eFuse.Type != Settings.Devices.Remote) {
		RMT::SetRXChannel(Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38, RMT_CHANNEL_2, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
		RMT::ReceiveStart(RMT_CHANNEL_2);
	}

	SetValue(0, "Protocol");
	SetValue(0, "Signal");
	SetValue(0, "Raw");
	SetValue(0, "IsRepeated");
	SetValue(0, "RepeatPause");
	SetValue(0, "RepeatCount");
	SetValue(0, "Frequency");

	SetIsInited(true);
}

bool SensorIR_t::HasPrimaryValue() {
	return false;
}

bool SensorIR_t::ShouldUpdateInMainLoop() {
	return false;
}

void SensorIR_t::Update() {
	Values.clear();

	uint32_t SignalDetectionTime = Time::UptimeToUnixTime(SignalDetectedTime);

	if (Time::IsUptime(SignalDetectionTime) && Time::Offset > 0)
		SignalDetectionTime = Time::Unixtime() - (Time::Uptime() - SignalDetectionTime);

	SetValue(LastSignal.Protocol				, "Protocol");
	SetValue(LastSignal.Uint32Data				, "Signal");
	SetValue(0									, "Raw");
	SetValue((uint8_t)LastSignal.IsRepeated		, "IsRepeated");
	SetValue(0									, "RepeatPause");
	SetValue(0									, "RepeatCount");
	SetValue(LastSignal.GetProtocolFrequency()	, "Frequency");

	Updated = SignalDetectionTime;
}

string SensorIR_t::FormatValue(string Key) {
	if (Key == "Protocol")
		return Converter::ToHexString(Values[Key], 2);

	if (Key == "Signal")
	{
		if (LastSignal.Protocol == IR_PROTOCOL_RAW)
			return "";
		else if (LastSignal.MiscData > 0)
			return Converter::ToHexString(LastSignal.Uint32Data,8) + Converter::ToHexString(LastSignal.MiscData,4);
	}

	if (Key == "Raw")
		return LastSignal.GetRawSignal();

	if (Key == "IsRepeated") {
		// hack для обработки унарного оператора - иначе crash
		string IsRepeated = (LastSignal.IsRepeated) ? "1" : "0";
		return IsRepeated;
	}

	if (Key == "RepeatPause") {
		// hack для обработки унарного оператора - иначе crash
		string RepeatPause = (LastSignal.IsRepeated) ? Converter::ToString(LastSignal.RepeatPause) : "0";
		return RepeatPause;
	}

	if (Key == "RepeatCount")
		return Converter::ToString(LastSignal.RepeatCount);

	if (Key == "Frequency")
		return Converter::ToString(LastSignal.GetProtocolFrequency());

	return Converter::ToHexString(Values[Key], 8);
}

bool SensorIR_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
	if (SceneEventCode == 0xEE) {
		Storage_t::Item Item = Storage.Read(SceneEventOperand);

		map<string,string> DecodedValue = StorageDecode(Item.DataToString());
		if (DecodedValue.count("Signal") > 0) {
			IRLib IRSignal(DecodedValue["Signal"]);
			return IRLib::CompareIsIdentical(LastSignal, IRSignal);
		}

		ESP_LOGE("SensorIR","Can't find Data in memory");
		return false;
	}

	//if (SceneEventCode == 0x01 && ValueItem.Value == 0x1) return true;
	//if (SceneEventCode == 0xFF && ValueItem.Value == 0xFF) return true;

	return false;
};

string SensorIR_t::SummaryJSON() {
	return "{}";
};

void SensorIR_t::MessageStart() {
	SensorIR_t* IRSensorItem = (SensorIR_t*)Sensor_t::GetSensorByID(SensorIRID);
	if (IRSensorItem == nullptr) {
		ESP_LOGE("MessageStart", "IR sensor didnt find");
		return;
	}

	IRSensorItem->SignalDetectedTime = Time::Uptime();
	SensorIRCurrentMessage.clear();
};

bool SensorIR_t::MessageBody(int16_t Bit) {
	if (SensorIRCurrentMessage.size() > 0 && SensorIRCurrentMessage.back() <= -Settings.SensorsConfig.IR.SignalEndingLen)
		return false;

	if (Bit == 0 || abs(Bit) >= Settings.SensorsConfig.IR.Threshold) {
		SensorIRCurrentMessage.push_back(-Settings.SensorsConfig.IR.SignalEndingLen);
		return false;
	}

	SensorIRCurrentMessage.push_back(Bit);

	return true;
};

void SensorIR_t::MessageEnd() {
	SensorIR_t* IRSensorItem = (SensorIR_t*)Sensor_t::GetSensorByID(SensorIRID);
	if (IRSensorItem == nullptr) {
		ESP_LOGE("MessageEnd", "IR sensor didnt find");
		return;
	}

	if (SensorIRCurrentMessage.size() < 4)
	{
		SensorIRCurrentMessage.empty();
		return;
	}

	for (auto &Item : SensorIRCurrentMessage)
		if (abs(Item) <= Settings.SensorsConfig.IR.ValueThreshold) {
			SensorIRCurrentMessage.empty();
			return;
		}

	bool IsRepeatSignal = false;

	
	vector<int32_t> PotentialRepeatSignal = IRSensorItem->LastSignal.GetRawRepeatSignalForSending();

	if (IRSensorItem->LastSignal.Protocol != 0xFF && PotentialRepeatSignal.size() > 0 &&
		IRLib::CompareIsIdentical(PotentialRepeatSignal, SensorIRCurrentMessage))
		IsRepeatSignal = true;

	if (!IsRepeatSignal) {
		IRSensorItem->LastSignal.RepeatCount = 0;
		IRSensorItem->LastSignal.RepeatPause = 0;
		IRSensorItem->LastSignal.RawData = SensorIRCurrentMessage;
		IRSensorItem->LastSignal.ExternFillPostOperations();
	}
/*
	if (Settings.eFuse.DeviceID == 0x00000003 || Settings.eFuse.DeviceID == 0x00000004) {
		string Output = "";
		for (auto &Item : SensorIRCurrentMessage)
			Output = Output + Converter::ToString(Item) + " ";

		ESP_LOGI("Received", "%s", Output.c_str());
	}
*/
	SensorIRCurrentMessage.empty();

	if (IRSensorItem->LastSignal.RawData.size() >= Settings.SensorsConfig.IR.MinSignalLen)
		Log::Add(Log::Events::Sensors::IRReceived);

	if (SignalReceivedTimer == NULL) {
		esp_timer_create_args_t TimerArgs;
		::memset(&TimerArgs, 0, sizeof(TimerArgs));
		TimerArgs.callback  = &SignalReceivedCallback;
		TimerArgs.name  	= "SignalReceivedTimer";

		::esp_timer_create(&TimerArgs, &SignalReceivedTimer);
	}

	::esp_timer_stop(SignalReceivedTimer);
	::esp_timer_start_once(SignalReceivedTimer, Settings.SensorsConfig.IR.DetectionDelay);	
};

void SensorIR_t::SignalReceivedCallback(void *Param) {
	SensorIR_t* IRSensorItem = (SensorIR_t*)Sensor_t::GetSensorByID(SensorIRID);
	if (IRSensorItem == nullptr) {
		ESP_LOGE("SignalReceivedCallback", "IR sensor didnt find");
		return;
	}

	if (IRSensorItem->LastSignal.RawData.size() < Settings.SensorsConfig.IR.MinSignalLen)
		return;

	IRSensorItem->Update();

	Wireless.SendBroadcastUpdated(SensorIRID, Converter::ToHexString(static_cast<uint8_t>(IRSensorItem->LastSignal.Protocol),2));
	Automation.SensorChanged(SensorIRID);

	if (Settings.eFuse.Type == Settings.Devices.Remote)
		((DataRemote_t*)Data)->SetExternalStatusByIRCommand(IRSensorItem->LastSignal);

	if (IRSensorItem->LastSignal.Protocol == 0xFF) {
		string URL = Settings.ServerUrls.BaseURL + "/ac/match";
		string CRC = IRSensorItem->LastSignal.GetSignalCRC();

		if (CRC.size() > 96) CRC = CRC.substr(0, 96);

		ESP_LOGE("CRC", "%s", CRC.c_str());

		HTTPClient::HTTPClientData_t QueryData;
		QueryData.URL 		= URL;
		QueryData.Method 	= QueryType::POST;

		strcpy(SensorIRSignalCRCBuffer, CRC.c_str());
		QueryData.POSTData = string(SensorIRSignalCRCBuffer);

		QueryData.ReadStartedCallback 	= &ACCheckStarted;
		QueryData.ReadBodyCallback 		= &ACCheckBody;
		QueryData.ReadFinishedCallback	= &ACCheckFinished;

		HTTPClient::Query(QueryData, false, true);
	}
}

// AC check signal callbacks
void SensorIR_t::ACCheckStarted(const char *IP) {
	SensorIRACCheckBuffer = "";
}

bool SensorIR_t::ACCheckBody(char Data[], int DataLen, const char *IP) {
	SensorIRACCheckBuffer += string(Data, DataLen);

	return true;
};

void SensorIR_t::ACCheckFinished(const char *IP) {
	if (SensorIRACCheckBuffer.size() == 0)
		return;

	SensorIR_t* IRSensorItem = (SensorIR_t*)Sensor_t::GetSensorByID(SensorIRID);
	if (IRSensorItem == nullptr) {
		ESP_LOGE("ACCheckFinished", "IR sensor didnt find");
		return;
	}

	vector<string> Codesets = JSON(SensorIRACCheckBuffer).GetStringArray();

	SensorIRACCheckBuffer = "";

	if (Codesets.size() == 0) return;

	for (string CodesetItem : Codesets) {
		if (CodesetItem.size() != 8) continue;

		string CodesetString= CodesetItem.substr(0, 4);
		string StatusString	= CodesetItem.substr(4, 4);

		uint16_t Codeset 	= Converter::UintFromHexString<uint16_t>(CodesetString);
		uint16_t Status		= Converter::UintFromHexString<uint16_t>(StatusString);

		if (Codeset == 0) return;

		//ESP_LOGE("RECEIVED DATA", "%04X %04X", Codeset, Status);

		if (Settings.eFuse.Type == Settings.Devices.Remote)
			((DataRemote_t*)Data)->SetExternalStatusForAC(Codeset, Status);

		LocalMQTT_t::SendMessage(StatusString, "/ir/ac/" + CodesetString);
	}
}

void SensorIR_t::ACReadAborted(const char *IP) {
	SensorIR_t* IRSensorItem = (SensorIR_t*)Sensor_t::GetSensorByID(SensorIRID);
	if (IRSensorItem == nullptr) {
		ESP_LOGE("ACReadAborted", "IR sensor didnt find");
		return;
	}

	SensorIRACCheckBuffer = "";
}

string SensorIR_t::StorageEncode(map<string,string> Data) {
	string Result = "";

	if (!Data.count("protocol") && !Data.count("title") && !Data.count("signal") && Data.count("data"))
		return Data["data"];

	if (Data.count("title")) {
		vector<uint16_t> Title = Converter::ToUTF16Vector(Data["title"]);

		Data["title"] = "";

		while (Title.size()%2!=0) Title.push_back(0);

		if (Title.size() > 62)
		Title.erase(Title.begin() + 62, Title.end());

		Result += Converter::ToHexString(Title.size() * 2, 2);

		while (Title.size() > 0) {
			Result += Converter::ToHexString(Title.front(),4);
			Title.erase(Title.begin());
		}
	}
	else
		Result += "00";

	uint8_t Protocol = 0xF0;
	if (Data.count("protocol")) Protocol = (uint8_t)Converter::ToUint16(Data["protocol"]);
		Result += Converter::ToHexString(Protocol, 2);

	if (Protocol != 0xF0)
		while (Data["signal"].size() < 8) Data["signal"] = "0" + Data["signal"];

	if (Data.count("signal"))
		Result += Data["signal"];

	return Result;
}

map<string,string> SensorIR_t::StorageDecode(string DataString) {
	map<string,string> Result = map<string,string>();

	if (DataString.size() < 4)
		return Result;

	uint8_t TitleLength = (uint8_t)Converter::UintFromHexString<uint8_t>(DataString.substr(0,2));

	if (DataString.size() > TitleLength * 2 + 1) {
		for (uint8_t Pos = 0; Pos < TitleLength * 2; Pos+=4)
			Result["Title"] += "\\u" + DataString.substr(2+Pos,4);
	}
	else
		return Result;

	DataString = DataString.substr(2 + TitleLength * 2);

	if (DataString.size() > 1) {
		Result["Protocol"]	= DataString.substr(0,2);
		DataString = DataString.substr(2);
	}

	Result["Signal"] = DataString;

	return Result;
}


