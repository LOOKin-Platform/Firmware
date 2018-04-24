/*
*    SensorColor.cpp
*    SensorColor_t implementation
*
*/
class SensorColor_t : public Sensor_t {
  public:
	SensorColor_t() {
	    ID          = 0x84;
	    Name        = "RGBW";
	    EventCodes  = { 0x00, 0x02, 0x03, 0x04 };
    }

    void Update() override {
    		SetValue(ReceiveValue("Red")  , "Red");
    		SetValue(ReceiveValue("Green"), "Green");
    		SetValue(ReceiveValue("Blue") , "Blue");
    		SetValue(ReceiveValue("White"), "White");

    		double Color = (double)floor(GetValue("Red").Value * 255 * 255 + GetValue("Green").Value * 255 + GetValue("Blue").Value);

    		if (SetValue(Color)) {
    			WebServer->UDPSendBroadcastUpdated(ID, Converter::ToHexString(Color,6));
    			Automation->SensorChanged(ID);
    		}
    };

    double ReceiveValue(string Key = "Primary") override {
    		switch (GetDeviceTypeHex()) {
    			case DEVICE_TYPE_PLUG_HEX:
    				if (Key == "Red")     return GPIO::PWMValue(COLOR_PLUG_RED_PWMCHANNEL);
    				if (Key == "Green")   return GPIO::PWMValue(COLOR_PLUG_GREEN_PWMCHANNEL);
    				if (Key == "Blue")    return GPIO::PWMValue(COLOR_PLUG_BLUE_PWMCHANNEL);
    				if (Key == "White")   return GPIO::PWMValue(COLOR_PLUG_WHITE_PWMCHANNEL);
    				if (Key == "Primary") return (double)floor(GetValue("Red").Value * 255 * 255 + GetValue("Green").Value * 255 + GetValue("Blue").Value);
    				break;
    		}

    		return 0;
    };

    string FormatValue(string Key) override {
    		if (Key == "Primary")
    			return Converter::ToHexString(Values[Key].Value, 6);
    		else
    			return Converter::ToHexString(Values[Key].Value, 2);
    }

    bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
    		double Red    = GetValue("Red").Value;
    		double Green  = GetValue("Green").Value;
    		double Blue   = GetValue("Blue").Value;

    		// Установленная яркость равна значению в сценарии
    		if (SceneEventCode == 0x02 && SceneEventOperand == ToBrightness(Red, Green, Blue))
    			return true;

    		// Установленная яркость меньше значения в сценарии
    		if (SceneEventCode == 0x03 && SceneEventOperand > ToBrightness(Red, Green, Blue))
    			return true;

    		// Установленная яркость меньше значения в сценарии
    		if (SceneEventCode == 0x04 && SceneEventOperand < ToBrightness(Red, Green, Blue))
    			return true;

    		return false;
    }

    uint8_t ToBrightness(uint32_t Color) {
    		uint8_t oBlue    = Color&0x000000FF;
    		uint8_t oGreen   = (Color&0x0000FF00)>>8;
    		uint8_t oRed     = (Color&0x00FF0000)>>16;

    		return ToBrightness(oRed, oGreen, oBlue);
    }

    uint8_t ToBrightness(uint8_t Red, uint8_t Green, uint8_t Blue) {
    		return max(max(Red,Green), Blue);
    }
};





