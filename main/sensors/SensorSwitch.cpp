/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

class SensorSwitch_t : public Sensor_t {
	public:
		SensorSwitch_t() {
			if (GetIsInited()) return;

			ID          = 0x81;
			Name        = "Switch";
			EventCodes  = { 0x00, 0x01, 0x02 };

			SetIsInited(true);
		}

		void Update() override {
			if (SetValue(ReceiveValue(), "Primary"), 0) {
				Wireless.SendBroadcastUpdated(ID, Converter::ToString(GetValue()));
				Automation.SensorChanged(ID);
			}
		};

		uint32_t ReceiveValue(string Key = "Primary") override {
        	if (Settings.GPIOData.GetCurrent().Switch.GPIO == GPIO_NUM_0)
        	{
        		return 0;
        	}

			return (GPIO::Read(Settings.GPIOData.GetCurrent().Switch.GPIO) == true) ? 1 : 0;
		};

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
			uint32_t ValueItem = GetValue();

			if (SceneEventCode == 0x01 && ValueItem == 1) return true;
			if (SceneEventCode == 0x02 && ValueItem == 0) return true;

			return false;
		}
};
