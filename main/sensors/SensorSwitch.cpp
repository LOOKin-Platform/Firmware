/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

#include "SensorSwitch.h"

#include "HardwareIO.h"

#include "Wireless.h"
#include "Automation.h"

SensorSwitch_t::SensorSwitch_t() {
	if (GetIsInited()) return;

	ID          = 0x81;
	Name        = "Switch";
	EventCodes  = { 0x00, 0x01, 0x02 };

	SetIsInited(true);
}

void SensorSwitch_t::Update() {
	if (SetValue(ReceiveValue(), "Primary"), 0) {
		Wireless_t::SendBroadcastUpdated(ID, Converter::ToString(GetValue()));
		Automation_t::SensorChanged(ID);
	}
}

uint32_t SensorSwitch_t::ReceiveValue(string Key) {
	if (Settings.GPIOData.GetCurrent().Switch.GPIO == GPIO_NUM_MAX)
	{
		return 0;
	}

	return (GPIO::Read(Settings.GPIOData.GetCurrent().Switch.GPIO) == true) ? 1 : 0;
}

bool SensorSwitch_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
	uint32_t ValueItem = GetValue();

	if (SceneEventCode == 0x01 && ValueItem == 1) return true;
	if (SceneEventCode == 0x02 && ValueItem == 0) return true;

	return false;
}

