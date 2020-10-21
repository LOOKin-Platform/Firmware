/*
 *    Wireless.cpp
 *    Class to handle all wireless interfaces
 *
 */

#include "Wireless.h"
#include "Globals.h"

static char tag[] = "Wireless";

void Wireless_t::StartInterfaces() {
	ESP_LOGI(tag, "StartInterfaces");

	if (!WiFi.IsRunning()) {
		if (!Network.WiFiConnect("", IsFirstWiFiStart)) {
			WiFi.StartAP(Settings.WiFi.APSSID);
		}
	}
}

void Wireless_t::StopWiFi() {
	WiFi.Stop();
}

void Wireless_t::SendBroadcastUpdated(uint8_t SensorID, string Value, string Operand) {
	string UpdatedString = WebServer.UDPUpdatedBody(SensorID, Value, Operand);

	if (WiFi.IsRunning())
	{
		WebServer.UDPSendBroadcast(UpdatedString);

		if (MQTT.GetStatus() == MQTT_t::CONNECTED)
			MQTT.SendMessage(UpdatedString, "/devices/" + Device.IDToString() + "/UDP", 2);
	}
	else
	{
		WebServer.UDPSendBroadcast(UpdatedString);
		IsEventDrivenStart = true;
		StartInterfaces();
	}

	//BLEServer.StartAdvertising("1" + Converter::ToHexString(SensorID,2) + Value + AdditionalData);
}

bool Wireless_t::IsPeriodicPool() {
	uint16_t SecondsInHour	= (uint16_t)(Time::Unixtime() % 3600);

	uint16_t SleepInterval	= (uint16_t)Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first;
	uint16_t WakeupInterval	= (uint16_t)Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second;

	uint8_t	IntervalsPassed = (uint8_t)floor((float)SecondsInHour / (SleepInterval + WakeupInterval));

	if (SecondsInHour == IntervalsPassed*(SleepInterval + WakeupInterval) + SleepInterval)
		return true;
	else
		return false;
}
