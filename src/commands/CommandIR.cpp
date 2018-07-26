/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/
#ifndef COMMANDS_IR
#define COMMANDS_IR

#include <RMT.h>

static rmt_channel_t TXChannel = RMT_CHANNEL_4;

class CommandIR_t : public Command_t {
  public:
	CommandIR_t() {
		ID          = 0x07;
		Name        = "IR";

		Events["nec"]  		= 0x01;
		Events["saved"]		= 0xEE;
		Events["prontohex"]	= 0xF0;
		Events["raw"]		= 0xF1;

		if (Settings.GPIOData.GetCurrent().IR.SenderGPIO != GPIO_NUM_0)
			RMT::SetTXChannel(Settings.GPIOData.GetCurrent().IR.SenderGPIO, TXChannel, 38000);
	}

    bool Execute(uint8_t EventCode, string StringOperand) override {

    		uint32_t Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

    		if (EventCode == 0x01) {
    			IRLib IRSignal;
    			IRSignal.Protocol 	= IRLib::ProtocolEnum::NEC;
    			IRSignal.Uint32Data = Operand;
    			IRSignal.FillRawData();

    			RMT::TXSetItems(IRSignal.RawData);
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

        				if (DecodedValue.count("Protocol") > 0) {
            				IRSignal.Protocol = static_cast<IRLib::ProtocolEnum>((uint8_t)Converter::ToUint16(DecodedValue["Protocol"]));
        				}

            			RMT::TXSetItems(IRSignal.RawData);
            			RMT::TXSend(TXChannel, IRSignal.Frequency);
        			}
        			else {
        				ESP_LOGE("CommandIR","Can't find Data in memory");
        				return false;
        			}
    			}
    		}

    		if (EventCode == 0xF0) {
    			IRLib IRSignal(StringOperand);
    			RMT::TXSetItems(IRSignal.RawData);
    			RMT::TXSend(TXChannel, IRSignal.Frequency);
    		}

    		if (EventCode == 0xF1) {
    			uint16_t	Frequency = 38000;
    			size_t 		FrequencyDelimeterPos = StringOperand.find(";");

    			if (FrequencyDelimeterPos != std::string::npos) {
    				Frequency = Converter::ToUint16(StringOperand.substr(0,FrequencyDelimeterPos));
    				StringOperand = StringOperand.substr(FrequencyDelimeterPos+1);
    			}

    			vector<string> Values = Converter::StringToVector(StringOperand, " ");

    			RMT::TXClear();

    			for (string Item : Values)
    				RMT::TXAddItem(Converter::ToInt32(Item));

        		RMT::TXSend(RMT_CHANNEL_4, Frequency);
    		}

    		return true;
    }
};

#endif
