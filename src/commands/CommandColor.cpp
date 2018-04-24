/*
*    CommandColor.cpp
*    CommandColor_t implementation
*
*/

class CommandColor_t : public Command_t {
  public:
    CommandColor_t() {
        ID          = 0x04;
        Name        = "RGBW";

        Events["color"]       = 0x01;
        Events["brightness"]  = 0x02;

        switch (GetDeviceTypeHex()) {
          case DEVICE_TYPE_PLUG_HEX:
            TimerIndex  = COLOR_PLUG_TIMER_INDEX;

            GPIORed     = COLOR_PLUG_RED_PIN_NUM;
            GPIOGreen   = COLOR_PLUG_GREEN_PIN_NUM;
            GPIOBlue    = COLOR_PLUG_BLUE_PIN_NUM;
            GPIOWhite   = COLOR_PLUG_WHITE_PIN_NUM;

            PWMChannelRED   = COLOR_PLUG_RED_PWMCHANNEL;
            PWMChannelGreen = COLOR_PLUG_GREEN_PWMCHANNEL;
            PWMChannelBlue  = COLOR_PLUG_BLUE_PWMCHANNEL;
            PWMChannelWhite = COLOR_PLUG_WHITE_PWMCHANNEL;
            break;

          case DEVICE_TYPE_REMOTE_HEX:
            TimerIndex  = COLOR_REMOTE_TIMER_INDEX;

            GPIORed     = COLOR_REMOTE_RED_PIN_NUM;
            GPIOGreen   = COLOR_REMOTE_GREEN_PIN_NUM;
            GPIOBlue    = COLOR_REMOTE_BLUE_PIN_NUM;
            GPIOWhite   = COLOR_REMOTE_WHITE_PIN_NUM;

            PWMChannelRED   = COLOR_REMOTE_RED_PWMCHANNEL;
            PWMChannelGreen = COLOR_REMOTE_GREEN_PWMCHANNEL;
            PWMChannelBlue  = COLOR_REMOTE_BLUE_PWMCHANNEL;
            PWMChannelWhite = COLOR_REMOTE_WHITE_PWMCHANNEL;
            break;
        }

        GPIO::SetupPWM(GPIORed  , TimerIndex, PWMChannelRED);
        GPIO::SetupPWM(GPIOGreen, TimerIndex, PWMChannelGreen);
        GPIO::SetupPWM(GPIOBlue , TimerIndex, PWMChannelBlue);
        GPIO::SetupPWM(GPIOWhite, TimerIndex, PWMChannelWhite);
    }

    void Overheated() override {
    		Execute(0x01, 0x00000000);
    }

    bool Execute(uint8_t EventCode, string StringOperand) override {
    		bool Executed = false;

    		uint32_t Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

    		if (EventCode == 0x01) {
    			uint8_t Red, Green, Blue = 0;

    			Blue    = Operand&0x000000FF;
    			Green   = (Operand&0x0000FF00)>>8;
    			Red     = (Operand&0x00FF0000)>>16;

    			//(Operand&0xFF000000)>>24;
    			//floor((Red  * Power) / 255)

    			if ((Blue == Green && Green == Red) && GPIOWhite != GPIO_NUM_0)
    				GPIO::PWMFadeTo(PWMChannelWhite, Blue);
    			else {
    				if (GPIORed   != GPIO_NUM_0) GPIO::PWMFadeTo(PWMChannelRED  , Red);
    				if (GPIOGreen != GPIO_NUM_0) GPIO::PWMFadeTo(PWMChannelGreen, Green);
    				if (GPIOBlue  != GPIO_NUM_0) GPIO::PWMFadeTo(PWMChannelBlue , Blue);
    			}

    			Executed = true;
    		}

    		if (EventCode == 0x02)
    			Executed = true;

    		if (Executed) {
    			ESP_LOGI("CommandColor_t", "Executed. Event code: %s, Operand: %s", Converter::ToHexString(EventCode, 2).c_str(), Converter::ToHexString(Operand, 8).c_str());
    			Sensor_t::GetSensorByID(ID + 0x80)->Update();
    			return true;
    		}

    		return false;
    }

  private:
    gpio_num_t      GPIORed, GPIOGreen, GPIOBlue, GPIOWhite;
    ledc_timer_t    TimerIndex;
    ledc_channel_t  PWMChannelRED, PWMChannelGreen, PWMChannelBlue, PWMChannelWhite;
};
