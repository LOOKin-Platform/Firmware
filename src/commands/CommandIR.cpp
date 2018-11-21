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
	CommandIR_t() {
		ID          = 0x07;
		Name        = "IR";

		Events["nec36"]  	= 0x01;
		Events["nec38"]  	= 0x02;
		Events["nec40"]  	= 0x03;
		Events["nec56"]  	= 0x04;
		Events["saved"]		= 0xEE;
		Events["prontohex"]	= 0xF0;
		Events["raw"]		= 0xFF;

		if (Settings.GPIOData.GetCurrent().IR.SenderGPIO != GPIO_NUM_0)
			RMT::SetTXChannel(Settings.GPIOData.GetCurrent().IR.SenderGPIO, TXChannel, 38000);
	}

    bool Execute(uint8_t EventCode, string StringOperand) override {
    		uint32_t Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

    		if (EventCode > 0x0 && EventCode < 0x05) {
    			IRLib IRSignal;
    			IRSignal.Protocol 	= 0x01;
    			IRSignal.Uint32Data = Operand;
    			IRSignal.FillRawData();

    			RMT::TXSetItems(IRSignal.RawData);

    			switch (EventCode) {
					case 0x01: IRSignal.Frequency = 36000; break;
					case 0x02: IRSignal.Frequency = 38000; break;
					case 0x03: IRSignal.Frequency = 40000; break;
					case 0x04: IRSignal.Frequency = 56000; break;
    			}

    			TXSend(IRSignal.Frequency);
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
            				IRSignal.Protocol = (uint8_t)Converter::ToUint16(DecodedValue["Protocol"]);
        				}

            			RMT::TXSetItems(IRSignal.RawData);
            			TXSend(IRSignal.Frequency);
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
    			TXSend(IRSignal.Frequency);
    		}

    		if (EventCode == 0xFF) {
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

    			TXSend(Frequency);
    		}

    		return true;
    }

    void TXSend(uint16_t Frequency) {
    	InOperation = true;

		RMT::TXSend(RMT_CHANNEL_4, Frequency);
		Log::Add(Log::Events::Commands::IRExecuted);

		InOperation = false;
    }
};

#endif
