#include "HandlersPooling.h"

#include <esp_log.h>
#include <DateTime.h>

#include <netdb.h>
#include <mdns.h>

#include "Matter.h"
#include "MatterWiFi.h"

#include "API.h"

#include "MyWiFiHandler.h"

static char HandlerTag[] = "WiFiHandler";

static bool					IsEventDrivenStart	= false;
static uint32_t				LastAPQueryTime 	= 0;

void HandlersPooling_t::WiFiUptimeHandler::SetClientModeNextTime(uint32_t Value) {
	ClientModeNextTime = ::Time::Unixtime() + Value;
}

void HandlersPooling_t::WiFiUptimeHandler::SetLastAPQueryTime() {
	LastAPQueryTime = ::Time::Unixtime();
}

void HandlersPooling_t::WiFiUptimeHandler::Start() {
	if (!WiFi.IsRunning()) {
		WiFiStartedTime = 0;
		Network.KeepWiFiTimer = 0;
		BatteryUptime = Settings.WiFi.BatteryUptime;
		Wireless.StartInterfaces();
		return;
	}
}

void HandlersPooling_t::WiFiUptimeHandler::Pool() {
	if (Device.PowerMode == DevicePowerMode::CONST ||
	   (Device.PowerMode == DevicePowerMode::BATTERY && !Device.SensorMode)) {
		BatteryUptime = Settings.WiFi.BatteryUptime;

		if (::Time::Unixtime() > ClientModeNextTime && WiFi_t::GetMode() == WIFI_MODE_APSTA_STR && ClientModeNextTime > 0)
		{
			MyWiFiEventHandler::IsConnectedBefore = false;
			ClientModeNextTime = 0;

			ESP_LOGE("Try to find wifi", "timer fired");

			if (Network.WiFiSettings.size() > 0) 
			{
				//WiFi.Stop();
				//FreeRTOS::Sleep(1000);

				Network.ImportScannedSSIDList(WiFi.Scan());
				Network.WiFiConnect();
				SetClientModeNextTime(Settings.WiFi.STAModeInterval);
			}
			else
				SetClientModeNextTime(Settings.WiFi.STAModeReconnect);
		}

		return;
	}

	if (WiFi.IsRunning()) {
		if (WiFiStartedTime == 0)
			WiFiStartedTime = ::Time::UptimeU();

		if (Network.KeepWiFiTimer > 0) 	{
			Network.KeepWiFiTimer -= Settings.Pooling.Interval;
			if (Network.KeepWiFiTimer < Settings.Pooling.Interval)
				Network.KeepWiFiTimer = Settings.Pooling.Interval;
		}

		if (BatteryUptime > 0) {
			if (::Time::UptimeU() >= WiFiStartedTime + BatteryUptime * 1000 && Network.KeepWiFiTimer <= 0) {
				WiFiStartedTime = 0;
				BatteryUptime = 0;
				WiFi.Stop();
			}
		}
		else {
			if (::Time::UptimeU() >= WiFiStartedTime + Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second * 1000000
					&& Network.KeepWiFiTimer <= 0) {
				ESP_LOGI("WiFiHandler", "WiFi uptime for battery device expired. Stopping wifi");
				WiFiStartedTime = 0;
				WiFi.Stop();
			}
		}
	}
	else {
		if (Wireless.IsPeriodicPool()) {
			IsEventDrivenStart = false;
			Wireless.StartInterfaces();
		}
	}
}