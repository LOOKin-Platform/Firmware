#include "HandlersPooling.h"
#include "CommandIR.h"
#include "CommandBLE.h"

void HandlersPooling_t::PingPeriodicHandler::FirmwareCheckStarted(const char *IP) {
	FirmwareUpdateURL = "";
}

bool HandlersPooling_t::PingPeriodicHandler::FirmwareCheckReadBody(char Data[], int DataLen, const char *IP) {
	const uint8_t MaxFirmwareUpdate = 255;

	if ((DataLen + FirmwareUpdateURL.size()) > MaxFirmwareUpdate)
		DataLen = MaxFirmwareUpdate - FirmwareUpdateURL.size();

	FirmwareUpdateURL +=  string(Data, DataLen);

	return true;
}

void HandlersPooling_t::PingPeriodicHandler::FirmwareCheckFinished(const char *IP) {
	bool ShouldCheckForReboot = true;

	if (FirmwareUpdateURL.size() > 10) {
		if (FirmwareUpdateURL.rfind("http", 0) == 0) {
			ShouldCheckForReboot = false;
			Device.OTAStart(FirmwareUpdateURL);
		}
	}

	if (ShouldCheckForReboot)
		CheckForPeriodicalyReboot();
}

void HandlersPooling_t::PingPeriodicHandler::FirmwareCheckFailed(const char *IP) {
	CheckForPeriodicalyReboot();
}

void HandlersPooling_t::PingPeriodicHandler::CheckForPeriodicalyReboot() {
	if (::Time::Uptime() > 26*60*60)
		BootAndRestore::Reboot(false);
}


void HandlersPooling_t::PingPeriodicHandler::Pool() {
	if (Device.Type.IsBattery() && Device.SensorMode == true)
		return;

	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	if (Device.Status != DeviceStatus::RUNNING)
		return;

	/*
	ESP_LOGE("HandlersPooling_t::PingPeriodicHandler", "%s", (Time::IsUptime(Time::Unixtime())) ? "Uptime" : "None");
	if (Time::Uptime() == 40) {
		ESP_LOGE("UPTIME", "%d", Time::Uptime());
		Time::TimezoneOffset+=2;
		RemotePingRestartCounter = Settings.Pooling.ServerPingInterval;
	}
	*/

	RemotePingRestartCounter += Settings.Pooling.Interval;
	RouterPingRestartCounter += Settings.Pooling.Interval;

	if (RemotePingRestartCounter >= Settings.Pooling.ServerPingInterval) {
		RemotePingRestartCounter = 0;
		RouterPingRestartCounter = 0;

		if (::Time::IsUptime(::Time::Unixtime()))
			::Time::ServerSync(Settings.ServerUrls.SyncTime);
		else
		{
			DateTime_t CurrentTime = ::Time::DateTime();

			if (CurrentTime.Hours == 3 && CurrentTime.Minutes < (Settings.Pooling.ServerPingInterval / 60*1000) && Device_t::IsAutoUpdate == true)
				HTTPClient::Query(Settings.ServerUrls.FirmwareCheck, QueryType::GET, true, true, &HandlersPooling_t::PingPeriodicHandler::FirmwareCheckStarted, &HandlersPooling_t::PingPeriodicHandler::FirmwareCheckReadBody, &HandlersPooling_t::PingPeriodicHandler::FirmwareCheckFinished, &HandlersPooling_t::PingPeriodicHandler::FirmwareCheckFailed);
			else
			{
				JSON TelemetryData;

				TelemetryData.SetItem("Uptime"		, Converter::ToString<uint32_t>(::Time::Uptime()));
				TelemetryData.SetItem("IsBattery"	, (Device.PowerMode == DevicePowerMode::BATTERY) ? "1" : "0");
				TelemetryData.SetItem("RCStatus"	, RemoteControl.GetStatusString());
				TelemetryData.SetItem("Memory"		, Converter::ToString(::esp_get_free_heap_size()));
				TelemetryData.SetItem("IsAutoUpdate", (Device.IsAutoUpdate) ? "true" : "false");
				TelemetryData.SetItem("capabilities", Converter::ToHexString(Device.CapabilitiesRaw(),4));

				CommandIR_t* CommandIR = (CommandIR_t*)Command_t::GetCommandByName("IR");
				if (CommandIR != nullptr) {
					TelemetryData.SetItem("IRSended", Converter::ToString<uint32_t>(CommandIR->SendCounter));
					CommandIR->SendCounter = 0;
				}

				CommandBLE_t* CommandBLE= (CommandBLE_t*)Command_t::GetCommandByName("BLE");
				if (CommandBLE != nullptr) {
					TelemetryData.SetItem("BLESended", Converter::ToString<uint32_t>(CommandBLE->SendCounter));
					CommandBLE->SendCounter = 0;
				}

				HTTPClient::HTTPClientData_t QueryData;
				QueryData.URL 		= Settings.ServerUrls.Telemetry;
				QueryData.Method 	= QueryType::POST;
				QueryData.POSTData 	= TelemetryData.ToString();

				HTTPClient::Query(QueryData, true, true);
			}
		}
	}
	else if (RouterPingRestartCounter >= Settings.Pooling.RouterPingInterval) {
		RouterPingRestartCounter = 0;

		PerformLocalPing();
	}
}

void HandlersPooling_t::PingPeriodicHandler::PerformLocalPing() {
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
	cbs.on_ping_end 	= HandlersPooling_t::PingPeriodicHandler::RouterPingFinished;
	cbs.cb_args 		= NULL;  // arguments that will feed to all callback functions, can be NULL

	esp_ping_handle_t PingHandler;
	esp_ping_new_session(&ping_config, &cbs, &PingHandler);
	esp_ping_start(PingHandler);

	IsRouterPingFinished.Wait("IsRouterPingFinished");

	esp_ping_stop(PingHandler);
	esp_ping_delete_session(PingHandler);
}

void HandlersPooling_t::PingPeriodicHandler::RouterPingFinished(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    printf("%lu packets transmitted, %lu received, time %lums\n", transmitted, received, total_time_ms);

    IsRouterPingFinished.Give();
}