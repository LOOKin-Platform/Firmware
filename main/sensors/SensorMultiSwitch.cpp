/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

#include "SensorMultiSwitch.h"

#include "HardwareIO.h"

#include "Automation.h"
#include "Wireless.h"

extern Wireless_t 	Wireless;
extern Automation_t	Automation;


SensorMultiSwitch_t::SensorMultiSwitch_t() {
	if (GetIsInited()) return;

	ID          = 0x82;
	Name        = "MultiSwitch";
	EventCodes  = { 0x00 };

	GPIOS = Settings.GPIOData.GetCurrent().MultiSwitch.GPIO;

	for (int i=0; i < GPIOS.size(); i++) {
		if (GPIOS[i] != GPIO_NUM_0) {
			GPIO::Setup(GPIOS[i]);
			EventCodes.push_back((i+1)*16);
			EventCodes.push_back(((i+1)*16)+1);
		}
	}

	SetIsInited(true);
}

void SensorMultiSwitch_t::Update() {
	SetValue(ReceiveValue("Primary"), "Primary");

	for (int i=0; i < GPIOS.size(); i++)
		if (GPIOS[i] != GPIO_NUM_0)
		{
			string Channel = "channel" + Converter::ToString<uint8_t>(i+1);
			uint32_t Value = ReceiveValue(Channel);

			if (Value != GetValue(Channel))
			{
				Wireless.SendBroadcastUpdated(ID, Converter::ToHexString((uint64_t)((i+1) * 16 + Value), 2));
				Automation.SensorChanged(ID);
			}

			SetValue(Value, Channel);
		}
}

uint32_t SensorMultiSwitch_t::ReceiveValue(string Key) {
	if (Key == "Primary") {
		bool AllOn = false;

		for (gpio_num_t Pin : GPIOS)
			if (GPIO::Read(Pin) == true) AllOn = true;

		return AllOn;
	}

	if (Key.size() <= 7)
		return 0;

	uint8_t ChannelNum = Converter::ToUint8(Key.substr(7)); //channelX

	if (ChannelNum > 0 && ChannelNum < GPIOS.size() + 1)
		return (GPIO::Read(GPIOS[ChannelNum-1]) == true) ? 1 : 0;

	return 0;
}

bool SensorMultiSwitch_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
	if (SceneEventCode < 16)
		return ((uint8_t)(GetValue() + 1) == SceneEventCode);

	uint8_t ChannelNum 	= (uint8_t)(SceneEventCode / 16);
	uint8_t State		= (uint8_t)(SceneEventCode % 16);

	if ((uint8_t)GetValue("channel" + ChannelNum) == State)
		return true;

	return false;
}