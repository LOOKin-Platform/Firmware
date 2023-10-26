/*
*    CommandColor.cpp
*    CommandColor_t implementation
*
*/

#include "CommandRGBW.h"

#include "Sensors.h"
#include "HardwareIO.h"

CommandColor_t::CommandColor_t() {
	ID          = 0x04;
	Name        = "RGBW";

	Events["color"]       = 0x01;
	Events["brightness"]  = 0x02;

	Settings_t::GPIOData_t::Color_t GPIO = Settings.GPIOData.GetCurrent().Color;

/*
	Не работает, обновить до поддержки обновленного HardwareIO
	if (GPIO.Red.GPIO	!= GPIO_NUM_0) GPIO::SetupPWM(GPIO.Red.GPIO		, GPIO.Timer, GPIO.Red.Channel	);
	if (GPIO.Green.GPIO	!= GPIO_NUM_0) GPIO::SetupPWM(GPIO.Green.GPIO	, GPIO.Timer, GPIO.Green.Channel);
	if (GPIO.Blue.GPIO	!= GPIO_NUM_0) GPIO::SetupPWM(GPIO.Blue.GPIO	, GPIO.Timer, GPIO.Blue.Channel	);
	if (GPIO.White.GPIO	!= GPIO_NUM_0) GPIO::SetupPWM(GPIO.White.GPIO	, GPIO.Timer, GPIO.White.Channel);
*/
	if (GPIO.Red.GPIO != GPIO_NUM_0 || GPIO.Green.GPIO	!= GPIO_NUM_0 || GPIO.Blue.GPIO	!= GPIO_NUM_0 || GPIO.White.GPIO != GPIO_NUM_0)
		GPIO::PWMFadeInstallFunction();
}

void CommandColor_t::Overheated() {
	Execute(0x01, 0x00000000);
}

bool CommandColor_t::Execute(uint8_t EventCode,  const char* StringOperand) {
	bool Executed = false;

	Settings_t::GPIOData_t::Color_t GPIO = Settings.GPIOData.GetCurrent().Color;

	uint32_t Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

	if (EventCode == 0x01) {
		uint8_t Red, Green, Blue = 0;

		Blue    = Operand&0x000000FF;
		Green   = (Operand&0x0000FF00)>>8;
		Red     = (Operand&0x00FF0000)>>16;

		//(Operand&0xFF000000)>>24;
		//floor((Red  * Power) / 255)

		if ((GPIO.Blue.GPIO == GPIO.Green.GPIO && Green == GPIO.Red.GPIO) && GPIO.White.GPIO != GPIO_NUM_0)
			GPIO::PWMFadeTo(GPIO.White.Channel, Blue);
		else {
			if (GPIO.Red.GPIO   != GPIO_NUM_0) GPIO::PWMFadeTo(GPIO.Red.Channel		, Red);
			if (GPIO.Green.GPIO != GPIO_NUM_0) GPIO::PWMFadeTo(GPIO.Green.Channel	, Green);
			if (GPIO.Blue.GPIO  != GPIO_NUM_0) GPIO::PWMFadeTo(GPIO.Blue.Channel 	, Blue);
		}

		Executed = true;
	}

	if (EventCode == 0x02)
		Executed = true;

	if (Executed) {
		ESP_LOGI("CommandColor_t", "Executed. Event code: %s, Operand: %s", Converter::ToHexString(EventCode, 2).c_str(), Converter::ToHexString(Operand, 8).c_str());

		if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr)
			Sensor_t::GetSensorByID(ID + 0x80)->Update();
		return true;
	}

	return false;
}