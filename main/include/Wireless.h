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
		static void StartInterfaces();

		void 		StopInterfaces(bool ShouldBlockForever);
		void		StopWiFi();
		void 		StopBluetooth();

		static void	SendBroadcastUpdated(uint8_t SensorID, string Value, string Operand = "", bool IsScheduled = true, bool InvokeStartIntefaces = false);
		static void SendBroadcastUpdated(string ServiceID, string Value, string Operand = "", bool IsScheduled = true, bool InvokeStartIntefaces = false);
		bool 		IsPeriodicPool();

		uint8_t 	GetBLEStatus();

		static inline bool 		IsFirstWiFiStart	= true;
		static inline bool 		IsEventDrivenStart = false;
	private:
		static inline bool		BlockRestartForever = false;
};

#endif // WIRELESS_H
