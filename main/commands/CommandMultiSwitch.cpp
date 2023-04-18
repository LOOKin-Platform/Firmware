/*
*    CommandSwitch.cpp
*    CommandSwitch_t implementation
*
*/

#include "CommandMultiSwitch.h"

#include "HardwareIO.h"

#include "Sensors.h"

CommandMultiSwitch_t::CommandMultiSwitch_t() {
	ID          = 0x02;
	Name        = "MultiSwitch";

	GPIOS = Settings.GPIOData.GetCurrent().MultiSwitch.GPIO;

	for (uint8_t i=0; i < GPIOS.size(); i++) {
		if (GPIOS[i] != GPIO_NUM_0) {
			GPIO::Setup(GPIOS[i]); //GPIO::Setup(GPIOS[i], GPIO_MODE_OUTPUT);
			string EventName 	= "channel" + Converter::ToString<uint8_t>(i+1);
			Events[EventName] 	= i+1;
		}
	}

	Events["all"] = 0xFF;
}

void CommandMultiSwitch_t::Overheated() {
	Execute(0x0, "0");
}

bool CommandMultiSwitch_t::IsOn(string Operand)
{
	return ((Converter::ToLower(Operand) == "on" || Operand == "1" || Operand == "01"));
}

bool CommandMultiSwitch_t::IsOff(string Operand)
{
	return ((Converter::ToLower(Operand) == "off" || Operand == "0" || Operand == "00"));
}

bool CommandMultiSwitch_t::Execute(uint8_t EventCode, const char *StringOperand) {
	bool Executed = false;

	if (EventCode == 0xFF) {
		for (gpio_num_t Pin : GPIOS)
			Executed = SetPin(Pin, StringOperand);
	}

	if (EventCode > 0 && EventCode < GPIOS.size() + 1)
		Executed = SetPin(GPIOS[EventCode - 1], StringOperand);

	if (Executed) {
		if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr)
			Sensor_t::GetSensorByID(ID + 0x80)->Update();
		return true;
	}

	return Executed;
}

bool CommandMultiSwitch_t::SetPin(gpio_num_t Pin, string StringOperand) {
	if (IsOn(StringOperand)) {
		GPIO::Hold(Pin, 0);
		GPIO::Write(Pin, 1);
		GPIO::Hold(Pin, 1);
		return true;
	}

	if (IsOff(StringOperand)) {
		GPIO::Hold(Pin, 0);
		GPIO::Write(Pin, 0);
		GPIO::Hold(Pin, 1);
		return true;
	}

	return false;
}
