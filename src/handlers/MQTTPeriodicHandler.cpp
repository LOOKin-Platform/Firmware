#ifndef MQTT_HANDLER
#define MQTT_HANDLER

class MQTTPeriodicHandler {
	public:
		static void Pool();
		static void ClearCounter();
	private:
		static uint32_t MQTTRestartCounter;
};

uint32_t MQTTPeriodicHandler::MQTTRestartCounter = 0;

void MQTTPeriodicHandler::ClearCounter() {
	MQTTRestartCounter = 0;
}

void IRAM_ATTR MQTTPeriodicHandler::Pool() {
	return;

	if (Device.Type.IsBattery())
		return;

	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	MQTTRestartCounter += Settings.Pooling.Interval;

	if (MQTTRestartCounter >= Settings.Pooling.MQTTInterval) {
		MQTTRestartCounter = 0;

		if (MQTT.GetStatus() != MQTT_t::CONNECTED)
			MQTT.Reconnect();
	}
}

#endif
