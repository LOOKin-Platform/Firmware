#ifndef PING_HANDLER
#define PING_HANDLER

class PingPeriodicHandler {
	public:
		static void Pool();

		static void FirmwareCheckStarted(char IP[]);
		static bool FirmwareCheckReadBody(char Data[], int DataLen, char IP[]);
		static void FirmwareCheckFinished(char IP[]);
		static void OTAFailed();

		static void	ExecuteOTATask	(void*);

	private:
		static TaskHandle_t	OTATaskHandler;
		static uint32_t 	PingRestartCounter;
		static string		FirmwareUpdateURL;
};

uint32_t 		PingPeriodicHandler::PingRestartCounter = 0;
string 			PingPeriodicHandler::FirmwareUpdateURL = "";
TaskHandle_t	PingPeriodicHandler::OTATaskHandler = NULL;

void PingPeriodicHandler::FirmwareCheckStarted(char IP[]) {
	FirmwareUpdateURL = "";
}

bool PingPeriodicHandler::FirmwareCheckReadBody(char Data[], int DataLen, char IP[]) {
	const uint8_t MaxFirmwareUpdate = 255;

	if ((DataLen + FirmwareUpdateURL.size()) > MaxFirmwareUpdate)
		DataLen = MaxFirmwareUpdate - FirmwareUpdateURL.size();

	FirmwareUpdateURL +=  string(Data, DataLen);

	return true;
}

void PingPeriodicHandler::FirmwareCheckFinished(char IP[]) {
	ESP_LOGE("URL TO OTA UPDATE", "%s", FirmwareUpdateURL.c_str());

	if (FirmwareUpdateURL.size() == 0)
		return;

	Device.Status = DeviceStatus::UPDATING;

	if (OTATaskHandler == NULL)
		OTATaskHandler = FreeRTOS::StartTask(PingPeriodicHandler::ExecuteOTATask, "ExecuteOTATask", NULL, 10240, 7);
}

void PingPeriodicHandler::OTAFailed() {
	ESP_LOGE("OTA UPDATE FAILED", "");

	RemoteControl.Start();
	LocalMQTT.Start();

	HomeKit::Start();

	if (OTATaskHandler != NULL) {
		FreeRTOS::DeleteTask(OTATaskHandler);
		OTATaskHandler = NULL;
	}
}

void PingPeriodicHandler::ExecuteOTATask(void*) {
	ESP_LOGE("OTA UPDATE STARTED", "");

	LocalMQTT.Stop();
	RemoteControl.Stop();

	HomeKit::Stop();

	FreeRTOS::Sleep(5000);

	OTA::Update(FirmwareUpdateURL, NULL, OTAFailed);
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
		else
		{
			DateTime_t CurrentTime = Time::DateTime();

			if (CurrentTime.Hours == 3 && CurrentTime.Minutes < (Settings.Pooling.PingInterval / 60*1000))
				HTTPClient::Query(Settings.ServerUrls.FirmwareCheck, QueryType::GET, true, &PingPeriodicHandler::FirmwareCheckStarted, &PingPeriodicHandler::FirmwareCheckReadBody, &PingPeriodicHandler::FirmwareCheckFinished, NULL);
			else
				HTTPClient::Query(Settings.ServerUrls.Ping, QueryType::GET, true, NULL, NULL, NULL, NULL);
		}
	}
}

#endif
