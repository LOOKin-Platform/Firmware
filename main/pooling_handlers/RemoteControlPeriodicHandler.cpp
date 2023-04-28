#include "HandlersPooling.h"

void HandlersPooling_t::RemoteControlPeriodicHandler::Pool() {
	return;

	if (Device.Type.IsBattery() && Device.SensorMode == true)
		return;

	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	MQTTRestartCounter += Settings.Pooling.Interval;

	if (MQTTRestartCounter >= Settings.Pooling.MQTTInterval) {
		MQTTRestartCounter = 0;

		if (RemoteControl.GetStatus() != RemoteControl_t::CONNECTED)
			RemoteControl.Reconnect();
	}
}