/*
*    SensorButton.cpp
*    SensorButton_t implementation
*
*/

#include "Sensors.h"
#include "Commands.h"

#include "SensorButton.h"
#include "SensorSwitch.h"

#include "CommandSwitch.h"

#include "ISR.h"

#include "Automation.h"
#include "Wireless.h"

#include "HandlersPooling.h"

void SensorButton_t::SensorButtonTask(void *) {
	while (1)
	{
		uint64_t OperatedTime = Time::UptimeU() - Settings.SensorsConfig.TouchSensor.TaskDelay * 1000;

	    for (pair<gpio_num_t, uint64_t> Item : SensorButtonStatusMap) {
	    	vector<gpio_num_t>::iterator it = find(SensorButtonGPIOS.begin(), SensorButtonGPIOS.end(), Item.first);

	    	if (it == SensorButtonGPIOS.cend())
	    		{continue;}

	    	uint8_t ChannelNum = distance(SensorButtonGPIOS.begin(), it);
	    	uint8_t Status = (Item.second > OperatedTime) ? 1 : 0;

	    	Sensor_t::GetSensorByID(SensorButtonID)->SetValue(Status, "channel" + Converter::ToString(ChannelNum));

			//GPIO::Write(GPIO_NUM_25, Status);
	    }

		FreeRTOS::Sleep(Settings.SensorsConfig.TouchSensor.TaskDelay);
	}
}

void SensorButton_t::Callback(void* arg) {
	gpio_num_t ActiveGPIO = static_cast<gpio_num_t>((uint32_t) arg);

	if (SensorButtonStatusMap.find(ActiveGPIO) != SensorButtonStatusMap.end())
		SensorButtonStatusMap[ActiveGPIO] = Time::UptimeU();
	else
		SensorButtonStatusMap.insert(pair<gpio_num_t, uint64_t>(ActiveGPIO, Time::UptimeU()));


	if (Settings.eFuse.Type == Settings.Devices.Plug) {
		SensorSwitch_t* SensorSwitchItem  	= (SensorSwitch_t*)Sensor_t::GetSensorByID(0x81);

		if (SensorSwitchItem != nullptr) {			
			uint8_t Status = SensorSwitchItem->ReceiveValue();
			Status = (Status == 0) ? 1 : 2;

			HandlersPooling_t::ExecuteQueueAddFromISR(0x01, Status);
		} 
	}
}

SensorButton_t::SensorButton_t() {
	if (GetIsInited()) return;

	ID          = SensorButtonID;
	Name        = "Button";
	EventCodes  = { 0x00, 0x01, 0x02 };

	SensorButtonGPIOS = Settings.GPIOData.GetCurrent().Touch.GPIO;

	for (int i=0; i < SensorButtonGPIOS.size(); i++) {
		if (SensorButtonGPIOS[i] != GPIO_NUM_MAX) {
			ISR::AddInterrupt(SensorButtonGPIOS[i], GPIO_INTR_NEGEDGE, &Callback);

			EventCodes.push_back((i+1)*16);
			EventCodes.push_back(((i+1)*16)+1);
		}
	}

	if (SensorButtonGPIOS.size() > 0) {
		ISR::Install();
		FreeRTOS::StartTask(SensorButtonTask, "SensorButtonTask", NULL, 2048);
	}

	//GPIO::Setup(GPIO_NUM_25);

	SetIsInited(true);
}

void SensorButton_t::Update() {
	if (SetValue(ReceiveValue(), "Primary"), 0) {
		Wireless_t::SendBroadcastUpdated(ID, Converter::ToString(GetValue()));
		Automation_t::SensorChanged(ID);
	}
};

uint32_t SensorButton_t::ReceiveValue(string Key) {
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

bool SensorButton_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
	/*
	SensorValueItem ValueItem = GetValue();

	if (SceneEventCode == 0x01 && ValueItem.Value == 1) return true;
	if (SceneEventCode == 0x02 && ValueItem.Value == 0) return true;
	*/
	return false;
}