#ifndef PING_HANDLER
#define PING_HANDLER

class PingPeriodicHandler {
	public:
		static void Pool();

		static void FirmwareCheckStarted(const char *IP);
		static bool FirmwareCheckReadBody(char Data[], int DataLen, const char *IP);
		static void FirmwareCheckFinished(const char *IP);

	private:
		static FreeRTOS::Semaphore IsRouterPingFinished;
		static void PerformLocalPing();
		static void RouterPingFinished(esp_ping_handle_t hdl, void *args);

		static uint32_t 	RemotePingRestartCounter;
		static uint32_t 	RouterPingRestartCounter;
		static string		FirmwareUpdateURL;
};

uint32_t 			PingPeriodicHandler::RemotePingRestartCounter 	= 0;
uint32_t 			PingPeriodicHandler::RouterPingRestartCounter 	= 0;
string 				PingPeriodicHandler::FirmwareUpdateURL 			= "";
FreeRTOS::Semaphore	PingPeriodicHandler::IsRouterPingFinished 		= FreeRTOS::Semaphore("IsRouterPingFinished");


void PingPeriodicHandler::FirmwareCheckStarted(const char *IP) {
	FirmwareUpdateURL = "";
}

bool PingPeriodicHandler::FirmwareCheckReadBody(char Data[], int DataLen, const char *IP) {
	const uint8_t MaxFirmwareUpdate = 255;

	if ((DataLen + FirmwareUpdateURL.size()) > MaxFirmwareUpdate)
		DataLen = MaxFirmwareUpdate - FirmwareUpdateURL.size();

	FirmwareUpdateURL +=  string(Data, DataLen);

	return true;
}

void PingPeriodicHandler::FirmwareCheckFinished(const char *IP) {
	Device.OTAStart(FirmwareUpdateURL);
}

void PingPeriodicHandler::Pool() {
	if (Device.Type.IsBattery() && Device.SensorMode == true)
		return;

	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	if (Device.Status != DeviceStatus::RUNNING)
		return;

	RemotePingRestartCounter += Settings.Pooling.Interval;
	RouterPingRestartCounter += Settings.Pooling.Interval;

	/*
	if (RemotePingRestartCounter >= 1.5*60*1000)
	{
		RemotePingRestartCounter = 0;
		RouterPingRestartCounter = 0;

		HTTPClient::Query(Settings.ServerUrls.FirmwareCheck, QueryType::GET, true, false, &PingPeriodicHandler::FirmwareCheckStarted, &PingPeriodicHandler::FirmwareCheckReadBody, &PingPeriodicHandler::FirmwareCheckFinished, NULL);
	}

	return;
	*/

	if (RemotePingRestartCounter >= Settings.Pooling.ServerPingInterval) {
		RemotePingRestartCounter = 0;
		RouterPingRestartCounter = 0;

		if (Time::IsUptime(Time::Unixtime()))
			Time::ServerSync(Settings.ServerUrls.SyncTime);
		else
		{
			DateTime_t CurrentTime = Time::DateTime();

			if (CurrentTime.Hours == 3 && CurrentTime.Minutes < (Settings.Pooling.ServerPingInterval / 60*1000))
				HTTPClient::Query(Settings.ServerUrls.FirmwareCheck, QueryType::GET, true, false, &PingPeriodicHandler::FirmwareCheckStarted, &PingPeriodicHandler::FirmwareCheckReadBody, &PingPeriodicHandler::FirmwareCheckFinished, NULL);
			else
			{
				JSON TelemetryData;

				TelemetryData.SetItem("Uptime", Converter::ToString<uint32_t>(Time::Uptime()));
				TelemetryData.SetItem("Eco", (Device.GetEcoFromNVS()) ? "1" : "0");
				TelemetryData.SetItem("HomeKit", Device.HomeKitToString());
				TelemetryData.SetItem("IsBattery", (Device.PowerMode == DevicePowerMode::BATTERY) ? "1" : "0");
				TelemetryData.SetItem("RCStatus", RemoteControl.GetStatusString());
				TelemetryData.SetItem("Memory", Converter::ToString(::esp_get_free_heap_size()));

				HTTPClient::HTTPClientData_t QueryData;
				QueryData.URL 		=  Settings.ServerUrls.Telemetry;
				QueryData.Method 	= QueryType::POST;
				QueryData.POSTData 	= TelemetryData.ToString();

				HTTPClient::Query(QueryData, true);
			}
		}
	}
	else if (RouterPingRestartCounter >= Settings.Pooling.RouterPingInterval) {
		RouterPingRestartCounter = 0;

		PerformLocalPing();
	}
}

void PingPeriodicHandler::PerformLocalPing() {
	IsRouterPingFinished.Take("IsRouterPingFinished");

	ip_addr_t RouterServerIP;

	RouterServerIP.u_addr.ip4.addr = WiFi.GetIPInfo().ip.addr;
	RouterServerIP.type = IPADDR_TYPE_V4;

	esp_ping_config_t ping_config 	= ESP_PING_DEFAULT_CONFIG();
	ping_config.target_addr 		= RouterServerIP;
	ping_config.count 				= 3;
	ping_config.timeout_ms			= 1000;
	ping_config.interval_ms			= 500;

	esp_ping_callbacks_t cbs;
	cbs.on_ping_success = NULL;
	cbs.on_ping_timeout = NULL;
	cbs.on_ping_end 	= PingPeriodicHandler::RouterPingFinished;
	cbs.cb_args 		= NULL;  // arguments that will feed to all callback functions, can be NULL

	esp_ping_handle_t PingHandler;
	esp_ping_new_session(&ping_config, &cbs, &PingHandler);
	esp_ping_start(PingHandler);

	IsRouterPingFinished.Wait("IsRouterPingFinished");

	esp_ping_stop(PingHandler);
	esp_ping_delete_session(PingHandler);
}

void PingPeriodicHandler::RouterPingFinished(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    printf("%d packets transmitted, %d received, time %dms\n", transmitted, received, total_time_ms);

    IsRouterPingFinished.Give();
}

#endif
