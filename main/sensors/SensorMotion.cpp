/*
*    SensorMotion.cpp
*    SensorMotion_t implementation
*
*/

#include "SensorMotion.h"

#include "Wireless.h"
#include "Automation.h"
#include "Sensors.h"

extern Wireless_t 	Wireless;
extern Automation_t	Automation;

void SensorMotion_t::MotionDetectTask(void *) {
	while (1)  {
		uint16_t Value = (uint16_t)adc1_get_raw(SensorMotionChannel);

		if (Value > 2000) {
			Sensor_t::GetSensorByID(SensorMotionID)->SetValue(1);
			Wireless_t::SendBroadcastUpdated(SensorMotionID, Converter::ToString(Sensor_t::GetSensorByID(SensorMotionID)->GetValue()));
			Automation_t::SensorChanged(SensorMotionID);

			while (Value > 2000) {
				Value = (uint16_t)adc1_get_raw(SensorMotionChannel);
				vTaskDelay(Settings.GPIOData.GetCurrent().Motion.PoolInterval);
			}

			Sensor_t::GetSensorByID(SensorMotionID)->SetValue(0);
		}
		else {
			vTaskDelay(Settings.GPIOData.GetCurrent().Motion.PoolInterval);
		}
	}
}

SensorMotion_t::SensorMotion_t() {
	if (GetIsInited()) return;

	ID          = SensorMotionID;
	Name        = "Motion";
	EventCodes  = { 0x00, 0x01 };

	if (Settings.GPIOData.GetCurrent().Motion.ADCChannel != ADC1_CHANNEL_MAX)
		SensorMotionChannel = Settings.GPIOData.GetCurrent().Motion.ADCChannel;

	if (SensorMotionChannel != ADC1_CHANNEL_MAX) {
		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(SensorMotionChannel, atten);
		FreeRTOS::StartTask(MotionDetectTask, "MotionDetectTask", NULL, 4096);
	}

	SetIsInited(true);
}

void SensorMotion_t::Update() {
	if (SetValue(ReceiveValue())) {
		Wireless_t::SendBroadcastUpdated(ID, Converter::ToString(GetValue()));
		Automation_t::SensorChanged(ID);
	}
}

uint32_t SensorMotion_t::ReceiveValue(string Key) {
	return ((uint16_t)adc1_get_raw(SensorMotionChannel) > 2000) ? 1 : 0;
};

bool SensorMotion_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
	uint32_t ValueItem = GetValue();

	if (SceneEventCode == 0x01 && ValueItem == 1)
		return true;

	return false;
}