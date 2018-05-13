/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/
#include <RMT.h>

class CommandIR_t : public Command_t {
  public:
	CommandIR_t() {
		ID          = 0x07;
		Name        = "IR";

		Events["nec"]  		= 0x01;
		Events["prontohex"]	= 0xEE;

		switch (GetDeviceTypeHex()) {
			case Settings.Devices.Remote:
				if (IR_REMOTE_SENDER_GPIO != GPIO_NUM_0)
					//RMT::SetTXChannel(IR_REMOTE_SENDER_GPIO, RMT_CHANNEL_4, 36000);
					RMT::SetTXChannel(IR_REMOTE_SENDER_GPIO, RMT_CHANNEL_4, 38000);
					//RMT::SetTXChannel(IR_REMOTE_SENDER_GPIO, RMT_CHANNEL_6, 40000);
					//RMT::SetTXChannel(IR_REMOTE_SENDER_GPIO, RMT_CHANNEL_7, 56000);
			break;
	    }
	}

    bool Execute(uint8_t EventCode, string StringOperand) override {

    		uint32_t Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

    		if (EventCode == 0x01) {
    			IRLib IRSignal;
    			IRSignal.Protocol 	= IRLib::ProtocolEnum::NEC;
    			IRSignal.Uint32Data 	= Operand;
    			IRSignal.FillRawData();

    			RMT::SetItems(IRSignal.RawData);
    		}

    		if (EventCode == 0xEE) {
    			IRLib IRSignal(StringOperand);
    			RMT::SetItems(IRSignal.RawData);
    		}

    		RMT::Send(RMT_CHANNEL_4);

    		return true;
    }
};
