#ifndef PING_HANDLER
#define PING_HANDLER

class PingPeriodicHandler {
	public:
		static void Pool();

	private:
		static uint32_t PingRestartCounter;
};

uint32_t PingPeriodicHandler::PingRestartCounter = 0;

void IRAM_ATTR PingPeriodicHandler::Pool() {
	if (Device.Type.IsBattery() && Device.SensorMode == true)
		return;

	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	PingRestartCounter += Settings.Pooling.Interval;

	if (PingRestartCounter >= Settings.Pooling.PingInterval) {
		PingRestartCounter = 0;

		if (Time::IsUptime(Time::Unixtime()))
			Time::ServerSync(Settings.ServerUrls.SyncTime);
		else
			HTTPClient::Query(Settings.ServerUrls.Ping, QueryType::GET, true, NULL, NULL, NULL, NULL);
	}
}

#endif
