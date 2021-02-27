/*
*    SensorTouch.cpp
*    SensorTouch_t implementation
*
*/

static map<gpio_num_t, uint64_t> 	SensorTouchStatusMap 	= {};
static vector<gpio_num_t> 			SensorTouchGPIOS 		= vector<gpio_num_t>();
static uint8_t 						SensorTouchID 			= 0xE2;

class SensorTouch_t;

static void SensorTouchTask(void *) {
	while (1)
	{
		uint64_t OperatedTime = Time::UptimeU() - Settings.SensorsConfig.TouchSensor.TaskDelay * 1000;

	    for (pair<gpio_num_t, uint64_t> Item : SensorTouchStatusMap) {
	    	vector<gpio_num_t>::iterator it = find(SensorTouchGPIOS.begin(), SensorTouchGPIOS.end(), Item.first);

	    	if (it == SensorTouchGPIOS.cend())
	    		continue;

	    	uint8_t ChannelNum = distance(SensorTouchGPIOS.begin(), it);

	    	uint8_t Status = (Item.second > OperatedTime) ? 1 : 0;

	    	Sensor_t::GetSensorByID(SensorTouchID)->SetValue(Status, "channel" + Converter::ToString(ChannelNum));

			//GPIO::Write(GPIO_NUM_25, Status);
	    }

		FreeRTOS::Sleep(Settings.SensorsConfig.TouchSensor.TaskDelay);
	}
}

class SensorTouch_t : public Sensor_t {
	private:
	public:
		static void Callback(void* arg) {
		    gpio_num_t ActiveGPIO = static_cast<gpio_num_t>((uint32_t) arg);

		    if (SensorTouchStatusMap.find(ActiveGPIO) != SensorTouchStatusMap.end())
		    	SensorTouchStatusMap[ActiveGPIO] = Time::UptimeU();
		    else
		    	SensorTouchStatusMap.insert(pair<gpio_num_t, uint64_t>(ActiveGPIO, Time::UptimeU()));
		}

		SensorTouch_t() {
			if (GetIsInited()) return;

			ID          = SensorTouchID;
			Name        = "Touch";
			EventCodes  = { 0x00, 0x01, 0x02 };

			SensorTouchGPIOS = Settings.GPIOData.GetCurrent().Touch.GPIO;

			for (int i=0; i < SensorTouchGPIOS.size(); i++) {
				if (SensorTouchGPIOS[i] != GPIO_NUM_0) {
					ISR::AddInterrupt(SensorTouchGPIOS[i], GPIO_INTR_ANYEDGE, &Callback);

					EventCodes.push_back((i+1)*16);
					EventCodes.push_back(((i+1)*16)+1);
				}
			}

			if (SensorTouchGPIOS.size() > 0) {
				ISR::Install();
				FreeRTOS::StartTask(SensorTouchTask, "SensorTouchTask", NULL, 2048);
			}

			//GPIO::Setup(GPIO_NUM_25);

			SetIsInited(true);
		}

		void Update() override {
			if (SetValue(ReceiveValue(), "Primary"), 0) {
				Wireless.SendBroadcastUpdated(ID, Converter::ToString(GetValue()));
				Automation.SensorChanged(ID);
			}
		};

		uint32_t ReceiveValue(string Key = "Primary") override {
			/*
			SensorSwitchSettingsItem CurrentSettings;

			if (SensorSwitchSettings.count(GetDeviceTypeHex()))
				CurrentSettings = SensorSwitchSettings[GetDeviceTypeHex()];

			if (CurrentSettings.GPIO != GPIO_NUM_0)
				return (GPIO::Read(CurrentSettings.GPIO) == true) ? 1 : 0;
			else
				return 0;
				*/

			return 0;
		};

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
			/*
			SensorValueItem ValueItem = GetValue();

			if (SceneEventCode == 0x01 && ValueItem.Value == 1) return true;
			if (SceneEventCode == 0x02 && ValueItem.Value == 0) return true;
			*/
			return false;
		}
};
