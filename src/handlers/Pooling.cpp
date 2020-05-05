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

			/*
			Command_t* CommandIR = Command_t::GetCommandByID(0x07);
			if (CommandIR != nullptr) {
				string Operand = "FA10B511";
				CommandIR->Execute(01, Operand);
			}
			*/

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
