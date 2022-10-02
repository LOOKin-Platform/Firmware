/*
 *    Wireless.cpp
 *    Class to handle all wireless interfaces
 *
 */

#include "Wireless.h"
#include "Globals.h"
#include "HomeKit.h"

static char tag[] = "Wireless";

void Wireless_t::StartInterfaces() {
	if (BlockRestartForever)
		return;

	if (Device.Status == UPDATING)
		return;

	ESP_LOGI(tag, "StartInterfaces");

	if (!WiFi.IsRunning()) {
		if (!Network.WiFiConnect("", IsFirstWiFiStart)) {
			WiFi.StartAP(Settings.WiFi.APSSID);
		}
	}
}

void Wireless_t::StopInterfaces(bool ShouldBlockForever) {
	if (ShouldBlockForever)
		BlockRestartForever = ShouldBlockForever;

	RemoteControl.Stop();
	LocalMQTT.Stop();

	if (HomeKit::IsEnabledForDevice())
		HomeKit::Stop();

	if (WiFi.IsRunning())
		Wireless.StopWiFi();

	if (BLEServer.IsRunning())
		Wireless.StopBluetooth();
}


void Wireless_t::StopWiFi() {
	WiFi.Stop();
}

void Wireless_t::StopBluetooth() {
	BLEServer.StopAdvertising();
}

void Wireless_t::SendBroadcastUpdated(uint8_t SensorID, string EventID, string Operand, bool IsScheduled, bool InvokeStartIntefaces) {
	SendBroadcastUpdated(Converter::ToHexString(SensorID, 2), EventID, Operand, IsScheduled);
}

void Wireless_t::SendBroadcastUpdated(string SensorOrServiceID, string EventID, string Operand, bool IsScheduled, bool InvokeStartIntefaces) {
	if (Device.Status == UPDATING)
		return;

	string UpdatedString = WebServer.UDPUpdatedBody(SensorOrServiceID, EventID, Operand);

	if (WiFi.IsRunning())
	{
		WebServer.UDPSendBroadcast(UpdatedString, IsScheduled);

		if (LocalMQTT.GetStatus() == LocalMQTT_t::CONNECTED) {
			Sensor_t* Sensor = Sensor_t::GetSensorByID(Converter::UintFromHexString<uint8_t>(SensorOrServiceID));

			if (Sensor != nullptr)
				LocalMQTT.SendMessage(Sensor->RootSensorJSON(), "/sensors/" + Converter::ToLower(Sensor->Name));
		}

		if (RemoteControl.GetStatus() == RemoteControl_t::CONNECTED)
			RemoteControl.SendMessage(UpdatedString, "/devices/" + Device.IDToString() + "/UDP", 2);
	}
	else
	{
		WebServer.UDPSendBroadcast(UpdatedString, IsScheduled);
		IsEventDrivenStart = true;
		
		if (InvokeStartIntefaces)
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

uint8_t Wireless_t::GetBLEStatus() {
	return BLEServer.GetStatus();
}
