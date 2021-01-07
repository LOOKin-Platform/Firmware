#ifndef MQTT_HANDLER
#define MQTT_HANDLER

class RemoteControlPeriodicHandler {
	public:
		static void Pool();
		static void ClearCounter();
	private:
		static uint32_t MQTTRestartCounter;
};

uint32_t RemoteControlPeriodicHandler::MQTTRestartCounter = 0;

void RemoteControlPeriodicHandler::ClearCounter() {
	MQTTRestartCounter = 0;
}

void RemoteControlPeriodicHandler::Pool() {
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

#endif
