/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

class SensorSwitch_t : public Sensor_t {
  public:
    SensorSwitch_t() {
        ID          = 0x81;
        Name        = "Switch";
        EventCodes  = { 0x00, 0x01, 0x02 };
    }

    void Update() override {
    	if (SetValue(ReceiveValue())) {
    		WebServer.UDPSendBroadcastUpdated(ID, Converter::ToString(GetValue().Value));
   			Automation.SensorChanged(ID);
   		}
    };

    uint32_t ReceiveValue(string Key = "Primary") override {
   		switch (GetDeviceTypeHex()) {
   			case Settings.Devices.Plug:
    			return (GPIO::Read(SWITCH_PLUG_PIN_NUM) == true) ? 1 : 0;
    			break;
   			default: return 0;
   		}
    };

    bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
    	SensorValueItem ValueItem = GetValue();

    	if (SceneEventCode == 0x01 && ValueItem.Value == 1) return true;
    	if (SceneEventCode == 0x02 && ValueItem.Value == 0) return true;

    	return false;
    }
};
