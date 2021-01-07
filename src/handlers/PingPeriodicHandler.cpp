#ifndef PING_HANDLER
#define PING_HANDLER

class PingPeriodicHandler {
	public:
		static void Pool();
		static bool FirmwareCheckReadBody(char Data[], int DataLen, char IP[]);

	private:
		static uint32_t PingRestartCounter;
};

uint32_t PingPeriodicHandler::PingRestartCounter = 0;

bool PingPeriodicHandler::FirmwareCheckReadBody(char Data[], int DataLen, char IP[]) {
	string OTAURL = string(Data, DataLen);
	ESP_LOGE("ESPOTAURL", "%s", OTAURL.c_str());
	OTA::Update(OTAURL);

	return true;
}

void PingPeriodicHandler::Pool() {
	if (Device.Type.IsBattery() && Device.SensorMode == true)
		return;

	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	PingRestartCounter += Settings.Pooling.Interval;

	if (PingRestartCounter >= Settings.Pooling.PingInterval) {
		PingRestartCounter = 0;

		if (Time::IsUptime(Time::Unixtime()))
			Time::ServerSync(Settings.ServerUrls.SyncTime);
		else {
			DateTime_t CurrentTime = Time::DateTime();

			if (CurrentTime.Hours == 3 && CurrentTime.Minutes < (Settings.Pooling.PingInterval / 60*1000))
				HTTPClient::Query(Settings.ServerUrls.FirmwareCheck, QueryType::GET, true, NULL, &PingPeriodicHandler::FirmwareCheckReadBody, NULL, NULL);
			else
				HTTPClient::Query(Settings.ServerUrls.Ping, QueryType::GET, true, NULL, NULL, NULL, NULL);
		}
	}
}

#endif
