/*
*    SensorIR.cpp
*    SensorIR_t implementation
*
*/

#include <IRLib.h>
#include <ISR.h>
#include "HTTPClient.h"

#include "Data.h"

extern DataEndpoint_t *Data;

static vector<int32_t> 		SensorIRCurrentMessage 			= {};
static uint8_t 				SensorIRID 						= 0x87;

static IRLib 				LastSignal;
static IRLib				FollowingSignal;
static string				RepeatCode;

static uint64_t 			LastSignalEnd 	= 0;
static uint64_t				NewSignalStart	= 0;
static uint64_t 			NewSignalEnd	= 0;

static uint16_t				RepeatPause 	= 0;

static esp_timer_handle_t 	SignalReceivedTimer = NULL;

static string 				SensorIRACCheckBuffer 	= "";

static char 				SensorIRSignalCRCBuffer[96] = "\0";

class SensorIR_t : public Sensor_t {
	public:
		SensorIR_t() {
			if (GetIsInited()) return;

			ID          = SensorIRID;
			Name        = "IR";
			EventCodes  = { 0x00, 0x01, 0xEE, 0xFF };

			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0) {
				RMT::SetRXChannel(Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38, RMT_CHANNEL_2, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
				RMT::ReceiveStart(RMT_CHANNEL_2);
			}

			const esp_timer_create_args_t TimerArgs = {
				.callback 			= &SignalReceivedCallback,
				.arg 				= NULL,
				.dispatch_method 	= ESP_TIMER_TASK,
				.name				= "SignalReceivedTimer",
				.skip_unhandled_events = false
			};

			::esp_timer_create(&TimerArgs, &SignalReceivedTimer);

			SetValue(0, "Protocol");
			SetValue(0, "Signal");
			SetValue(0, "Raw");
			SetValue(0, "IsRepeated");
			SetValue(0, "RepeatSignal");
			SetValue(0, "RepeatPause");

			SetIsInited(true);
		}

		bool HasPrimaryValue() override {
			return false;
		}

		bool ShouldUpdateInMainLoop() override {
			return false;
		}

		void Update() override {
			Values.clear();

			uint32_t SignalDetectionTime = Time::UptimeToUnixTime((uint32_t)(NewSignalEnd / 1000));

			if (Time::IsUptime(SignalDetectionTime) && Time::Offset > 0)
				SignalDetectionTime = Time::Unixtime() - (Time::Uptime() - SignalDetectionTime);

			SetValue(LastSignal.Protocol			, "Protocol");
			SetValue(LastSignal.Uint32Data			, "Signal");
			SetValue(0								, "Raw");
			SetValue((uint8_t)LastSignal.IsRepeated	, "IsRepeated");
			SetValue(0								, "RepeatSignal");
			SetValue(0								, "RepeatPause");

			Updated = SignalDetectionTime;

		}

		string FormatValue(string Key = "Primary") override {
			if (Key == "Protocol")
				return Converter::ToHexString(Values[Key], 2);

			if (Key == "Signal" && LastSignal.Protocol == IR_PROTOCOL_RAW)
				return "0";

			if (Key == "Raw")
				return LastSignal.GetRawSignal();

			if (Key == "IsRepeated")
				return (LastSignal.IsRepeated) ? "1" : "0";

			if (Key == "RepeatSignal")
				return RepeatCode;

			if (Key == "RepeatPause")
				return (LastSignal.IsRepeated) ? Converter::ToString(RepeatPause) : "0";

			return Converter::ToHexString(Values[Key], 8);
		}

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
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

		string SummaryJSON() override {
			return "{}";
		};

		static void MessageStart() {
			NewSignalEnd 	= RMT::GetSignalEndU();
			NewSignalStart	= NewSignalEnd;

			SensorIRCurrentMessage.clear();
			RepeatCode = "";
		};

		static bool MessageBody(int16_t Bit) {
			if (SensorIRCurrentMessage.size() > 0 && SensorIRCurrentMessage.back() <= -Settings.SensorsConfig.IR.SignalEndingLen)
				return false;

			if (Bit == 0 || abs(Bit) >= Settings.SensorsConfig.IR.Threshold) {
				SensorIRCurrentMessage.push_back(-Settings.SensorsConfig.IR.SignalEndingLen);
				return false;
			}

			SensorIRCurrentMessage.push_back(Bit);
			NewSignalStart -= abs(Bit);

			return true;
		};

		static void MessageEnd() {
			for (auto &Item : SensorIRCurrentMessage)
				if (abs(Item) <= Settings.SensorsConfig.IR.ValueThreshold) {
					SensorIRCurrentMessage.empty();
					return;
				}

			bool IsOdd = false;

			uint64_t Length = NewSignalEnd - NewSignalStart;

			if (SensorIRCurrentMessage.size() <= Settings.SensorsConfig.IR.MinSignalLen && Length < 1500) {
				if (RepeatCode == "" && SensorIRCurrentMessage.size() >= 2) {
					for (int i=0; i < SensorIRCurrentMessage.size(); i++)
						RepeatCode += Converter::ToString(SensorIRCurrentMessage[i]) + ((i != SensorIRCurrentMessage.size() - 1) ? " " : "");

					IsOdd = true;
				}

				if (SensorIRCurrentMessage.size() < 2)
					IsOdd = true;

				if (IsOdd) {
					NewSignalEnd = LastSignalEnd;
					RMT::ClearQueue();
					return;
				}
			}

			LastSignal.IsRepeated = false;

			RepeatCode = "";

			uint64_t Pause = (LastSignalEnd > 0) ? NewSignalStart - LastSignalEnd : 0;

			FollowingSignal = IRLib(SensorIRCurrentMessage);

			bool IsFollowingRepeatSignal = false;
			if (LastSignal.Protocol != 0xFF && FollowingSignal.CompareIsIdenticalWith(LastSignal.GetRawRepeatSignalForSending()))
				IsFollowingRepeatSignal = true;

			if (!IsFollowingRepeatSignal)
				if (IRLib::CompareIsIdentical(LastSignal,FollowingSignal))
					IsFollowingRepeatSignal = true;

			if (!IsFollowingRepeatSignal) {
				if (Pause > 0 && Pause < Settings.SensorsConfig.IR.SignalPauseMax)
				{
					if (LastSignal.RawData.size() > 0)
						LastSignal.RawData.back() = -(uint32_t)Pause;

					LastSignal.AppendRawSignal(FollowingSignal);
				}
				else
					LastSignal = FollowingSignal;
			}
			else if (Pause <= Settings.SensorsConfig.IR.SignalPauseMax) {
				LastSignal.IsRepeated = true;

				RepeatPause = (Pause > numeric_limits<uint16_t>::max()) ? numeric_limits<uint16_t>::max() :  Pause;
			}

			FollowingSignal = IRLib();

			SensorIRCurrentMessage.empty();

			if (LastSignal.RawData.size() >= Settings.SensorsConfig.IR.MinSignalLen)
				Log::Add(Log::Events::Sensors::IRReceived);

			LastSignalEnd = NewSignalEnd;

			::esp_timer_stop(SignalReceivedTimer);
			::esp_timer_start_once(SignalReceivedTimer, Settings.SensorsConfig.IR.DetectionDelay);
		};


		static void SignalReceivedCallback(void *Param) {
			RMT::ClearQueue();

			if (LastSignal.RawData.size() < Settings.SensorsConfig.IR.MinSignalLen)
				return;

			Sensor_t* Sensor = Sensor_t::GetSensorByID(SensorIRID);
			Sensor->Update();

			Wireless.SendBroadcastUpdated(SensorIRID, Converter::ToHexString(static_cast<uint8_t>(LastSignal.Protocol),2));
			Automation.SensorChanged(SensorIRID);

			if (Settings.eFuse.Type == Settings.Devices.Remote)
				((DataRemote_t*)Data)->SetExternalStatusByIRCommand(LastSignal);

			if (LastSignal.Protocol == 0xFF) {
				string URL = Settings.ServerUrls.BaseURL + "/ac/match";
				string CRC = LastSignal.GetSignalCRC();

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
		static void ACCheckStarted(const char *IP) {
			SensorIRACCheckBuffer = "";
		}

		static bool ACCheckBody(char Data[], int DataLen, const char *IP) {
			SensorIRACCheckBuffer += string(Data, DataLen);
			return true;
		};

		static void ACCheckFinished(const char *IP) {
			if (SensorIRACCheckBuffer.size() == 0)
				return;

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

				LocalMQTTSend(StatusString, "/ir/ac/" + CodesetString);
			}
		}

		static void ACReadAborted(const char *IP) {
			SensorIRACCheckBuffer = "";
		}

		string StorageEncode(map<string,string> Data) override {
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

		map<string,string> StorageDecode(string DataString) override {
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
};


