#ifndef POOLING
#define POOLING

#include "OverheatHandler.cpp"
#include "WiFiHandler.cpp"
#include "BluetoothHandler.cpp"
#include "EnergyHandler.cpp"
#include "SensorPeriodicHandler.cpp"
#include "MQTTPeriodicHandler.cpp"
#include "PingPeriodicHandler.cpp"
#include "NetworkMapHandler.cpp"

class Pooling_t: public Task {
	void Run(void *data) override {

		WiFiUptimeHandler::Start();

		while (1) {
			if (Time::Uptime() % 10 == 0)
				ESP_LOGE("Pooling","RAM left %d", esp_get_free_heap_size());

			OverheatHandler			::Pool();
			WiFiUptimeHandler		::Pool();
			BluetoothPeriodicHandler::Pool();
			EnergyPeriodicHandler	::Pool();
			SensorPeriodicHandler	::Pool();
			MQTTPeriodicHandler		::Pool();
			PingPeriodicHandler		::Pool();
			NetworkMapHandler		::Pool();

			FreeRTOS::Sleep(Settings.Pooling.Interval);
		}
	} // End run
};

#endif
