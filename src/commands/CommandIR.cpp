/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/
#ifndef COMMANDS_IR
#define COMMANDS_IR

#include <RMT.h>
#include "Sensors.h"

static rmt_channel_t TXChannel = RMT_CHANNEL_4;

static string 	IRACReadBuffer 	= "";
static uint16_t	IRACFrequency	= 38000;

class CommandIR_t : public Command_t {
	public:
		struct LastSignal_t {
			uint8_t 	Protocol;
			uint32_t 	Data;
			uint16_t 	Misc;
		};

		CommandIR_t() {
			ID          			= 0x07;
			Name        			= "IR";

			Events["nec1"]			= 0x01;
			Events["sony"]			= 0x03;
			Events["necx"]			= 0x04;
			Events["panasonic"]		= 0x05;
			Events["samsung36"]		= 0x06;

			/*
			Events["daikin"]		= 0x08;
			Events["mitsubishi-ac"] = 0x09;
			Events["gree"] 			= 0x0A;
			Events["haier-ac"] 		= 0x0B;
			*/


			Events["aiwa"]			= 0x14;

			Events["repeat"]		= 0xED;

			Events["saved"]			= 0xEE;
			Events["ac"]			= 0xEF;
			Events["prontohex"]		= 0xF0;
			Events["raw"]			= 0xFF;
		}

		LastSignal_t LastSignal;

		bool Execute(uint8_t EventCode, string &StringOperand) override {
			uint16_t Misc 		= 0x0;
			uint32_t Operand 	= 0x0;

			if (StringOperand.size() > 8)
			{
				string ConverterOperand = (StringOperand.size() < 20) ? StringOperand : "";

				while (ConverterOperand.size() < 12)
					ConverterOperand = "0" + ConverterOperand;

				if (ConverterOperand.size() > 12)
					ConverterOperand = ConverterOperand.substr(ConverterOperand.size() - 12);

				Misc 	= Converter::UintFromHexString<uint16_t>(ConverterOperand.substr(0,4));
				Operand = Converter::UintFromHexString<uint32_t>(ConverterOperand.substr(4,8));
			}
			else
				Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

			if (EventCode > 0x0 && EventCode < 0xED) { // /commands/ir/nec || /commands/ir/sirc ...

				IRLib IRSignal;
				IRSignal.Protocol 	= EventCode;
				IRSignal.Uint32Data = Operand;
				IRSignal.MiscData	= Misc;

				LastSignal.Protocol = EventCode;
				LastSignal.Data		= Operand;
				LastSignal.Misc		= Misc;

				if (Operand == 0x0 && Misc == 0x0)
					return false;

				RMT::TXSetItems(IRSignal.GetRawDataForSending());
				TXSend(IRSignal.GetProtocolFrequency());
				return true;
			}

			if (EventCode == 0xED) {
				uint8_t ProtocolID = Converter::ToUint8(StringOperand);

				if (ProtocolID == 0)
					ProtocolID = LastSignal.Protocol;

				IRLib IRSignal;
				IRSignal.Protocol 	= ProtocolID;
				IRSignal.Uint32Data = LastSignal.Data;
				IRSignal.MiscData	= LastSignal.Misc;

				vector<int32_t> RepeatedSignal = IRSignal.GetRawRepeatSignalForSending();
				if (RepeatedSignal.size() == 0)
					return false;

				RMT::TXSetItems(RepeatedSignal);
				TXSend(IRSignal.GetProtocolFrequency());
				return true;
			}

			if (EventCode == 0xEE) {
				uint16_t StorageItemID = Converter::ToUint16(StringOperand);

				if (StorageItemID <= Settings.Storage.Data.Size / Settings.Storage.Data.ItemSize) {
					Storage_t::Item Item = Storage.Read(StorageItemID);

					Sensor_t* SensorIR = Sensor_t::GetSensorByID(ID + 0x80);
					if (SensorIR == nullptr) {
						ESP_LOGE("CommandIR","Can't get IR sensor to decode message from memory");
						return false;
					}

					map<string,string> DecodedValue = SensorIR->StorageDecode(Item.DataToString());
					if (DecodedValue.count("Signal") > 0) {
						IRLib IRSignal(DecodedValue["Signal"]);

						if (DecodedValue.count("Protocol") > 0)
							IRSignal.Protocol = (uint8_t)Converter::ToUint16(DecodedValue["Protocol"]);

						LastSignal.Protocol 	= IRSignal.Protocol;
						LastSignal.Data 		= IRSignal.Uint32Data;
						LastSignal.Misc			= IRSignal.MiscData;

						RMT::TXSetItems(IRSignal.GetRawDataForSending());
						TXSend(IRSignal.Frequency);

						return true;
					}
					else {
						ESP_LOGE("CommandIR","Can't find Data in memory");
						return false;
					}
				}
				else {
					return false;
				}
			}

			if (EventCode == 0xEF) { // ac by codeset
				ACOperand ACData(Operand);

				ESP_LOGE("ACOperand", "Mode %s", Converter::ToString<uint8_t>(ACData.Mode).c_str());
				ESP_LOGE("ACOperand", "Temperature %s", Converter::ToString<uint8_t>(ACData.Temperature).c_str());
				ESP_LOGE("ACOperand", "FanMode %s", Converter::ToString<uint8_t>(ACData.FanMode).c_str());
				ESP_LOGE("ACOperand", "SwingMode %s", Converter::ToString<uint8_t>(ACData.SwingMode).c_str());

				if (ACData.Codeset == 0) {
					ESP_LOGE("CommandIR","Can't parse AC codeset");
					return false;
				}

				string ttt = Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery();
				ESP_LOGE("!!!", "%s", ttt.c_str());

				HTTPClient::Query(	Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery(),
									QueryType::POST, true,
									&ACReadStarted,
									&ACReadBody,
									&ACReadFinished,
									&ACReadAborted);

				return true;
			}

			if (EventCode == 0xF0) {
				if (StringOperand == "0" || InOperation)
					return false;

				IRLib *IRSignal = new IRLib(StringOperand);

				LastSignal.Protocol	= IRSignal->Protocol;
				LastSignal.Data 	= IRSignal->Uint32Data;
				LastSignal.Misc		= IRSignal->MiscData;

				RMT::TXSetItems(IRSignal->GetRawDataForSending());
				TXSend(IRSignal->Frequency);

				delete IRSignal;
				return true;
			}

			if (EventCode == 0xFF) {
				if (StringOperand == "0" || InOperation)
					return false;

				uint16_t	Frequency = 38000;
				size_t 		FrequencyDelimeterPos = StringOperand.find(";");

				if (FrequencyDelimeterPos != std::string::npos) {
					Frequency = Converter::ToUint16(StringOperand.substr(0,FrequencyDelimeterPos));
					StringOperand = StringOperand.substr(FrequencyDelimeterPos+1);
				}

				if (StringOperand.find(" ") == string::npos)
					return false;

				IRLib *IRSignal = new IRLib();

				while (StringOperand.size() > 0)
				{
					size_t Pos = StringOperand.find(" ");
					string Item = (Pos != string::npos) ? StringOperand.substr(0,Pos) : StringOperand;

					IRSignal->RawData.push_back(Converter::ToInt32(Item));

					if (Pos == string::npos || StringOperand.size() < Pos)
						StringOperand = "";
					else
						StringOperand.erase(0, Pos+1);
				}

				IRSignal->ExternFillPostOperations();

				LastSignal.Protocol = IRSignal->Protocol;
				LastSignal.Data 	= IRSignal->Uint32Data;
				LastSignal.Misc		= IRSignal->MiscData;

				if (LastSignal.Protocol != 0xFF)
					RMT::TXSetItems(IRSignal->GetRawDataForSending());
				else {
					RMT::TXClear();

					while (IRSignal->RawData.size())
						RMT::TXAddItem(IRSignal->RawPopItem());
				}

				TXSend(Frequency);

				delete IRSignal;
				return true;
			}

			return false;
		}

		void TXSend(uint16_t Frequency) {
			if ( RMT::TXItemsCount() == 0)
				return;

			if (Settings.GPIOData.GetCurrent().IR.SenderGPIO == GPIO_NUM_0)
				return;

			InOperation = true;

			ESP_LOGE("ITEMS COUNT", "%d", RMT::TXItemsCount());

			RMT::TXSend(Settings.GPIOData.GetCurrent().IR.SenderGPIO, Frequency);
			Log::Add(Log::Events::Commands::IRExecuted);

			InOperation = false;
		}

		// AC Codeset callbacks

		static void ACReadStarted(char IP[]) {
			RMT::TXClear();

			IRACFrequency 		= 38000;
			IRACReadBuffer 		= "";
		}

		static bool ACReadBody(char Data[], int DataLen, char IP[]) {
			IRACReadBuffer += string(Data, DataLen);

			size_t FrequencyDelimeterPos = IRACReadBuffer.find(";");
			if (FrequencyDelimeterPos != std::string::npos) {
				IRACFrequency 	= Converter::ToUint16(IRACReadBuffer.substr(0, FrequencyDelimeterPos));
				IRACReadBuffer 	= IRACReadBuffer.substr(FrequencyDelimeterPos+1);
			}

			if (IRACReadBuffer.find(" ") == string::npos)
				return true;

			while (IRACReadBuffer.size() > 0)
			{
				size_t Pos = IRACReadBuffer.find(" ");

				if (Pos != string::npos) {
					ESP_LOGE("taf", "%s", IRACReadBuffer.substr(0,Pos).c_str());
					RMT::TXAddItem(Converter::ToInt32(IRACReadBuffer.substr(0,Pos)));
					IRACReadBuffer.erase(0, Pos + 1);
				}
				else
					break;
			}

			return true;
		};

		static void ACReadFinished(char IP[]) {
			if (IRACReadBuffer.size() > 0)
				RMT::TXAddItem(Converter::ToInt32(IRACReadBuffer));

			CommandIR_t* CommandIR = (CommandIR_t*)Command_t::GetCommandByName("IR");
			if (CommandIR == nullptr)
				ESP_LOGE("CommandIR","Can't get IR command");
			else
				CommandIR->TXSend(IRACFrequency);

			IRACReadBuffer = "";
		}

		static void ACReadAborted(char IP[]) {
			IRACReadBuffer = "";

			RMT::TXClear();
			ESP_LOGE("CommandIR", "Failed to retrieve ac code from server");
		}
};

#endif
