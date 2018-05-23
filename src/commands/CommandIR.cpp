/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/
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

		switch (GetDeviceTypeHex()) {
			case Settings.Devices.Remote:
				if (IR_REMOTE_SENDER_GPIO != GPIO_NUM_0)
					RMT::SetTXChannel(IR_REMOTE_SENDER_GPIO, TXChannel, 38000);
			break;
	    }
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
