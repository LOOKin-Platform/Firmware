#ifndef POOLING
#define POOLING

#include "OverheatHandler.cpp"
#include "WiFiHandler.cpp"
#include "BluetoothHandler.cpp"
#include "EnergyHandler.cpp"
#include "SensorPeriodicHandler.cpp"
#include "NetworkMapHandler.cpp"
#include "PingPeriodicHandler.cpp"
#include "RemoteControlPeriodicHandler.cpp"
#include "WirelessPriorityHandler.cpp"

class Pooling_t {
	public:
		static void BARCheck() {
			EnergyPeriodicHandler::Pool(true);
		}

		static void Pool () {

			WiFiUptimeHandler::Start();

			while (1) {
				if (Time::Uptime() % 10 == 0)
					ESP_LOGI("Pooling","RAM left %d", esp_get_free_heap_size());

				OverheatHandler				::Pool();
				WiFiUptimeHandler			::Pool();
				//BluetoothPeriodicHandler	::Pool();
				EnergyPeriodicHandler		::Pool();
				WirelessPriorityHandler		::Pool();
				SensorPeriodicHandler		::Pool();
				RemoteControlPeriodicHandler::Pool();
				PingPeriodicHandler			::Pool();
				NetworkMapHandler			::Pool();

				FreeRTOS::Sleep(Settings.Pooling.Interval);

				/*
				if (Time::Uptime() % 100 == 0)
					FreeRTOS::GetRunTimeStats();
				*/
			}
		}
};

#endif
