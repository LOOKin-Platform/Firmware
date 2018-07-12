/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

struct SensorSwitchSettingsItem {
	gpio_num_t	GPIO;
	SensorSwitchSettingsItem(gpio_num_t sGPIO = GPIO_NUM_0) : GPIO(sGPIO) {};
};

static map<uint8_t, SensorSwitchSettingsItem> SensorSwitchSettings = {
	{ 0x03, { .GPIO = GPIO_NUM_23 }}
};

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
			if (SetValue(ReceiveValue())) {
				Wireless.SendBroadcastUpdated(ID, Converter::ToString(GetValue().Value));
				Automation.SensorChanged(ID);
			}
		};

		uint32_t ReceiveValue(string Key = "Primary") override {
			SensorSwitchSettingsItem CurrentSettings;

			if (SensorSwitchSettings.count(GetDeviceTypeHex()))
				CurrentSettings = SensorSwitchSettings[GetDeviceTypeHex()];

			if (CurrentSettings.GPIO != GPIO_NUM_0)
				return (GPIO::Read(CurrentSettings.GPIO) == true) ? 1 : 0;
			else
				return 0;
		};

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
			SensorValueItem ValueItem = GetValue();

			if (SceneEventCode == 0x01 && ValueItem.Value == 1) return true;
			if (SceneEventCode == 0x02 && ValueItem.Value == 0) return true;

			return false;
		}
};
