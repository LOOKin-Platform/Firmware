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

class CommandIR_t : public Command_t {
	public:
		struct LastSignal_t {
			uint8_t 	Protocol;
			uint32_t 	Data;
			uint16_t 	Misc;
		};

		CommandIR_t() {
			ID          		= 0x07;
			Name        		= "IR";

			Events["nec1"]		= 0x01;
			Events["sirc"]		= 0x03;
			Events["samsung"]	= 0x04;
			Events["panasonic"]	= 0x05;

			Events["repeat"]	= 0xED;

			Events["saved"]		= 0xEE;
			Events["prontohex"]	= 0xF0;
			Events["raw"]		= 0xFF;

			if (Settings.GPIOData.GetCurrent().IR.SenderGPIO != GPIO_NUM_0)
				RMT::SetTXChannel(Settings.GPIOData.GetCurrent().IR.SenderGPIO, TXChannel, 38000);
		}

		LastSignal_t LastSignal;

		bool Execute(uint8_t EventCode, string StringOperand) override {
			uint16_t Misc 		= 0x0;
			uint32_t Operand 	= 0x0;

			if (StringOperand.size() > 8)
			{
				string ConverterOperand = StringOperand;

				while (ConverterOperand.size() < 12)
					ConverterOperand = "0" + ConverterOperand;

				if (ConverterOperand.size() > 12)
					ConverterOperand = ConverterOperand.substr(ConverterOperand.size() - 12);

				Misc 	= Converter::UintFromHexString<uint16_t>(ConverterOperand.substr(0,4));
				Operand = Converter::UintFromHexString<uint32_t>(ConverterOperand.substr(4,8));
			}
			else
				Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

			if (EventCode > 0x0 && EventCode < 0xED) { // /commands/ir/nec || /commands/ir/nec2 || /commands/ir/sirc/
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

				vector<int32_t> RepeatedSignal = IRSignal.GetRawRepeatSignal();
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

						ESP_LOGI("ProntoHEX", "%s", IRSignal.GetProntoHex().c_str());

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

			if (EventCode == 0xF0) {
				if (StringOperand == "0")
					return false;

				IRLib IRSignal(StringOperand);

				LastSignal.Protocol	= IRSignal.Protocol;
				LastSignal.Data 	= IRSignal.Uint32Data;
				LastSignal.Misc		= IRSignal.MiscData;

				RMT::TXSetItems(IRSignal.GetRawDataForSending());
				TXSend(IRSignal.Frequency);
				return true;
			}

			if (EventCode == 0xFF) {
				if (StringOperand == "0")
					return false;

				uint16_t	Frequency = 38000;
				size_t 		FrequencyDelimeterPos = StringOperand.find(";");

				if (FrequencyDelimeterPos != std::string::npos) {
					Frequency = Converter::ToUint16(StringOperand.substr(0,FrequencyDelimeterPos));
					StringOperand = StringOperand.substr(FrequencyDelimeterPos+1);
				}

				IRLib IRSignal(Converter::StringToVector(StringOperand, " "));

				LastSignal.Protocol = IRSignal.Protocol;
				LastSignal.Data 	= IRSignal.Uint32Data;
				LastSignal.Misc		= IRSignal.MiscData;

				RMT::TXClear();

				ESP_LOGE("Protocol","%d", LastSignal.Protocol);
				ESP_LOGE("Data","%d", LastSignal.Data);

				for (int32_t Item : IRSignal.GetRawDataForSending())
					RMT::TXAddItem(Item);

				TXSend(Frequency);

				return true;
			}

			return false;
		}

    void TXSend(uint16_t Frequency) {
    	InOperation = true;

		RMT::TXSend(RMT_CHANNEL_4, Frequency);
		Log::Add(Log::Events::Commands::IRExecuted);

		InOperation = false;
    }
};

#endif
