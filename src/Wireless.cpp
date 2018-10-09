/*
 *    Wireless.cpp
 *    Class to handle all wireless interfaces
 *
 */

#include "Wireless.h"
#include "Globals.h"

static char tag[] = "Wireless";

void Wireless_t::StartInterfaces() {
	if (!WiFi.IsRunning()) {
		if (!Network.WiFiConnect("", IsFirstWiFiStart)) {
			WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
		}
	}

	IsFirstWiFiStart = false;
}

void Wireless_t::SendBroadcastUpdated(uint8_t SensorID, string Value, string AdditionalData) {
	#if !defined(CONFIG_BT_ENABLED)

	if (WiFi.IsRunning())
		WebServer.UDPSendBroadcastUpdated(SensorID, Value);
	else {
		WebServer.UDPSendBroadcastUpdated(SensorID, Value);
		IsEventDrivenStart = true;
		StartInterfaces();
	}

	#else

	if (WiFi.IsRunning())
		WebServer.UDPSendBroadcastUpdated(SensorID, Value);
	else
		BLEServer.StartAdvertising("1" + Converter::ToHexString(SensorID,2) + Value + AdditionalData);

	#endif
}

bool Wireless_t::IsPeriodicPool() {
	uint16_t SecondsInHour	= (uint16_t)(Time::Unixtime() % 3600);

	uint16_t SleepInterval	= (uint16_t)Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first;
	uint16_t WakeupInterval	= (uint16_t)Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second;

	uint8_t	IntervalsPassed = (uint8_t)floor((float)SecondsInHour / (SleepInterval + WakeupInterval));

	//ESP_LOGI(tag, "Seconds: %d, SleepInterval:%d, WakeupInterval:%d, FindedTime:%d", SecondsInHour, SleepInterval, WakeupInterval,IntervalsPassed*(SleepInterval + WakeupInterval) + SleepInterval);

	if (SecondsInHour == IntervalsPassed*(SleepInterval + WakeupInterval) + SleepInterval)
		return true;
	else
		return false;
}
