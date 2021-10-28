#ifndef POOLING
#define POOLING

#include "OverheatHandler.cpp"
#include "WiFiHandler.cpp"
#include "BluetoothHandler.cpp"
#include "EnergyHandler.cpp"
#include "SensorPeriodicHandler.cpp"
#include "NetworkMapHandler.cpp"
#include "ExternalTempHandler.cpp"
#include "PingPeriodicHandler.cpp"
#include "RemoteControlPeriodicHandler.cpp"

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

				/*
				Command_t* CommandIR = Command_t::GetCommandByID(0x07);
				if (CommandIR != nullptr) {
					string Operand = "FA10B511";
					CommandIR->Execute(01, Operand);
				}
				 */

				OverheatHandler				::Pool();
				WiFiUptimeHandler			::Pool();
				BluetoothPeriodicHandler	::Pool();
				EnergyPeriodicHandler		::Pool();
				SensorPeriodicHandler		::Pool();
				RemoteControlPeriodicHandler::Pool();
				PingPeriodicHandler			::Pool();
				NetworkMapHandler			::Pool();

				FreeRTOS::Sleep(Settings.Pooling.Interval);
			}
		}
};

#endif
