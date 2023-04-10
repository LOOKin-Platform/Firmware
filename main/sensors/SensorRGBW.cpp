/*
*    SensorColor.cpp
*    SensorColor_t implementation
*
*/

#include "SensorRGBW.h"

SensorColor_t::SensorColor_t() {
	if (GetIsInited()) return;

	ID          = 0x84;
	Name        = "RGBW";
	EventCodes  = { 0x00, 0x02, 0x03, 0x04 };

	SetIsInited(true);
}

void SensorColor_t::Update() {
	SetValue(ReceiveValue("Red")  , "Red");
	SetValue(ReceiveValue("Green"), "Green");
	SetValue(ReceiveValue("Blue") , "Blue");
	SetValue(ReceiveValue("White"), "White");

	double Color = (double)floor(GetValue("Red").Value * 255 * 255 + GetValue("Green").Value * 255 + GetValue("Blue").Value);

	if (SetValue(Color)) {
		Wireless.SendBroadcastUpdated(ID, Converter::ToHexString(Color,6));
		Automation.SensorChanged(ID);
	}
};

uint32_t SensorColor_t::ReceiveValue(string Key = "Primary") {
	Settings_t::GPIOData_t::Color_t GPIO = Settings.GPIOData.GetCurrent().Color;

	if (Key == "Red" 	&& GPIO.Red.GPIO != GPIO_NUM_0)		return GPIO::PWMValue(GPIO.Red.Channel	);
	if (Key == "Green"	&& GPIO.Green.GPIO != GPIO_NUM_0)	return GPIO::PWMValue(GPIO.Green.Channel);
	if (Key == "Blue"	&& GPIO.Blue.GPIO != GPIO_NUM_0)	return GPIO::PWMValue(GPIO.Blue.Channel	);
	if (Key == "White"	&& GPIO.White.GPIO != GPIO_NUM_0)	return GPIO::PWMValue(GPIO.White.Channel);
	if (Key == "Primary") return (double)floor(GetValue("Red").Value * 255 * 255 + GetValue("Green").Value * 255 + GetValue("Blue").Value);

	return 0;
};

string SensorColor_t::FormatValue(string Key) {
	if (Key == "Primary")
		return Converter::ToHexString(Values[Key].Value, 6);
	else
		return Converter::ToHexString(Values[Key].Value, 2);
}

bool SensorColor_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
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

uint8_t SensorColor_t::ToBrightness(uint32_t Color) {
	uint8_t oBlue    = Color&0x000000FF;
	uint8_t oGreen   = (Color&0x0000FF00)>>8;
	uint8_t oRed     = (Color&0x00FF0000)>>16;

	return ToBrightness(oRed, oGreen, oBlue);
}

uint8_t SensorColor_t::ToBrightness(uint8_t Red, uint8_t Green, uint8_t Blue) {
	return max(max(Red,Green), Blue);
}