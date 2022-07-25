/*
 *    Wireless.h
 *    Class to handle all wireless interfaces
 *
 */

#ifndef WIRELESS_H
#define WIRELESS_H

#include <string>

using namespace std;

class Wireless_t {
	public:
		void 	StartInterfaces();

		void 	StopInterfaces(bool ShouldBlockForever);
		void	StopWiFi();
		void 	StopBluetooth();

		void 	SendBroadcastUpdated(uint8_t SensorID, string Value, string Operand = "", bool IsScheduled = true);
		void 	SendBroadcastUpdated(string ServiceID, string Value, string Operand = "", bool IsScheduled = true);
		bool 	IsPeriodicPool();

		uint8_t GetBLEStatus();

		bool 	IsFirstWiFiStart	= true;
		bool 	IsEventDrivenStart = false;
	private:
		bool	BlockRestartForever = false;
};

#endif // WIRELESS_H
