/*
*    CommandSwitch.cpp
*    CommandSwitch_t implementation
*
*/

class CommandSwitch_t : public Command_t {
  public:
    CommandSwitch_t() {
        ID          = 0x01;
        Name        = "Switch";

		Events["on"]  = 0x01;
		Events["off"] = 0x02;

		if (Settings.GPIOData.GetCurrent().Switch.GPIO != GPIO_NUM_0)
			GPIO::Setup(Settings.GPIOData.GetCurrent().Switch.GPIO);
    }
    
    void Overheated() override {
    	  Execute(0x02, 0);
    }

    bool Execute(uint8_t EventCode, string StringOperand) override {
    		bool Executed = false;

    		if (EventCode == 0x00)
    			return false;

    		if (EventCode == 0x01) {
				GPIO::Write(Settings.GPIOData.GetCurrent().Switch.GPIO, true);
    			Executed = true;
    		}

    		if (EventCode == 0x02) {
    			GPIO::Write(Settings.GPIOData.GetCurrent().Switch.GPIO, false);
    			Executed = true;
    		}

    		if (Executed) {
    			//ESP_LOGI(tag, "Executed. Event code: %s, Operand: %s", Converter::ToHexString(EventCode, 2).c_str(), Converter::ToHexString(Operand, 8).c_str());
    			Sensor_t::GetSensorByID(ID + 0x80)->Update();
    			return true;
    		}

    		return false;
    }
};
