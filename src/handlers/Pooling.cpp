#ifndef POOLING
#define POOLING

#include "OverheatHandler.cpp"
#include "WiFiHandler.cpp"
#include "BluetoothHandler.cpp"
#include "EnergyHandler.cpp"

class Pooling_t: public Task {
	void Run(void *data) override {
		while (1) {
			if (Time::Uptime() % 10 == 0) {
				ESP_LOGI("Pooling","RAM left %d", esp_get_free_heap_size());
			}

			OverheatHandler			::Pool();
			WiFiUptimeHandler		::Pool();
			BluetoothPeriodicHandler::Pool();
			EnergyPeriodicHandler	::Pool();

			FreeRTOS::Sleep(Settings.Pooling.Interval);
		}
	} // End run
};

#endif