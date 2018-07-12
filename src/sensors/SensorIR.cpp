/*
*    SensorIR.cpp
*    SensorIR_t implementation
*
*/

#include <IRLib.h>
#include <ISR.h>

static vector<int32_t> 		SensorIRCurrentMessage 			= {};
static uint8_t 				SensorIRID 						= 0x87;
static constexpr uint8_t 	SensorIRQueueSize				= 30;
static QueueHandle_t 		SensorIRQueueFrequencyDetect	= FreeRTOS::Queue::Create(SensorIRQueueSize, sizeof(uint32_t));

static IRLib 	LastSignal;
static uint32_t SignalDetectionTime = 0;

class SensorIR_t : public Sensor_t {
	public:
		SensorIR_t() {
			if (GetIsInited()) return;

			ID          = SensorIRID;
			Name        = "IR";
			EventCodes  = { 0x00, 0x01 };

			ESP_LOGI("SensorID", "Init %X", ID);

			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0) {
				RMT::SetRXChannel(Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38, RMT_CHANNEL_0, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
				RMT::ReceiveStart(RMT_CHANNEL_0);
			}

			SetIsInited(true);
		}

		void Update() override {
			Values.clear();

			if (LastSignal.Protocol == IRLib::ProtocolEnum::RAW && LastSignal.RawData.size() == 0)
			{
				SetValue(0);
				return;
			}

			Values["Primary"]	= SensorValueItem(static_cast<uint8_t>(LastSignal.Protocol), SignalDetectionTime);
			Values["Signal"]	= SensorValueItem(LastSignal.Uint32Data, SignalDetectionTime);
			Values["Frequency"] = SensorValueItem(LastSignal.Frequency, SignalDetectionTime);
		};

		string FormatValue(string Key = "Primary") override {
			if (Key == "Primary")
				return Converter::ToHexString(Values[Key].Value, 2);

			if (Key == "Signal" && LastSignal.Protocol == IRLib::ProtocolEnum::RAW && LastSignal.RawData.size() != 0) {
				string SignalOutput = "";

				for (int i=0; i < LastSignal.RawData.size(); i++)
					SignalOutput += Converter::ToString(LastSignal.RawData[i]) + ((i != LastSignal.RawData.size() - 1) ? " " : "");

				return SignalOutput;
			}

			if (Key == "Frequency")
				return Converter::ToString(LastSignal.Frequency);

			return Converter::ToHexString(Values[Key].Value, 8);
		}

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
			// test data
			SensorValueItem ValueItem = GetValue();

			if (SceneEventCode == 0x01 && ValueItem.Value == 0x1) return true;
			if (SceneEventCode == 0xF1 && ValueItem.Value == 0xF1) return true;

			return false;
		};

		static void MessageStart() {
			SensorIRCurrentMessage.clear();
		};

		static void MessageBody(int16_t Bit) {
			if (SensorIRCurrentMessage.size() > 0 && SensorIRCurrentMessage.back() < -40000)
				return;

			if (Bit == 0) SensorIRCurrentMessage.push_back(-45000);
				else SensorIRCurrentMessage.push_back(Bit);
		};

		static void MessageEnd() {
			if (SensorIRCurrentMessage.size() <= 8) {
				FreeRTOS::Queue::Reset(SensorIRQueueFrequencyDetect);
				return;
			}

			LastSignal = IRLib(SensorIRCurrentMessage);
			SignalDetectionTime = Time::Unixtime();
			//LastSignal.SetFrequency(FrequencyDetectCalculate());

			SensorIRCurrentMessage.empty();

			Wireless.SendBroadcastUpdated(SensorIRID, Converter::ToHexString(static_cast<uint8_t>(LastSignal.Protocol),2));
			Automation.SensorChanged(SensorIRID);
		};

		string StorageEncode(map<string,string> Data) override {
			string Result = "";

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

			uint8_t TitleLength = (uint8_t)Converter::ToUint16(DataString.substr(0,2));

			if (DataString.size() > TitleLength * 2 + 1) {
				for (uint8_t Pos = 0; Pos < TitleLength * 2; Pos+=4) {
					Result["Title"] += "\\u" + DataString.substr(2+Pos,4);
				}
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
