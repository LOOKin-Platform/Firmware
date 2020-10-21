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
		void StartInterfaces();
		void StopWiFi();
		void SendBroadcastUpdated(uint8_t SensorID, string Value, string Operand = "");
		bool IsPeriodicPool();

		bool IsFirstWiFiStart	= true;
		bool IsEventDrivenStart = false;
	private:
};

#endif // WIRELESS_H
