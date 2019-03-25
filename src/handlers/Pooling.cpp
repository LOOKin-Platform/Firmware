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

			/*
			if (Time::Uptime() % 2 == 0) {
				Command_t::GetCommandByName("IR")->Execute(0xFF, "9020 -4470 600 -530 530 -580 520 -580 580 -530 610 -500 610 -500 610 -500 600 -1590 570 -1640 530 -1670 580 -1620 610 -1590 610 -1590 610 -1590 610 -1590 610 -530 580 -530 520 -1670 580 -530 610 -500 610 -500 610 -500 610 -530 580 -1610 530 -1670 530 -580 600 -1590 610 -1590 610 -1590 610 -1590 610 -1590 610 -530 520 -3310 8940 -2210 530 -4640 9090 -4500 590 -540 590 -520 610 -510 590 -540 590 -520 610 -510 560 -570 570 -1680 560 -1690 620 -1650 590 -1650 590 -1660 620 -1650 570 -1680 560 -1690 590 -540 590 -540 590 -1650 590 -540 590 -520 610 -510 560 -570 570 -540 620 -1650 590 -1650 590 -540 590 -1650 -45000");
			}
			*/

			OverheatHandler			::Pool();
			WiFiUptimeHandler		::Pool();
			BluetoothPeriodicHandler::Pool();
			EnergyPeriodicHandler	::Pool();

			FreeRTOS::Sleep(Settings.Pooling.Interval);
		}
	} // End run
};

#endif
