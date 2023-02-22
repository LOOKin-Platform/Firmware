#include "HandlersPooling.h"

void HandlersPooling_t::SensorPeriodicHandler::Pool() {
	if (Device.Status == UPDATING)
		return;

	//if (Time::Uptime() == 0) { // every 5 seconds pool sensors
	for (auto& Sensor : Sensors)
		Sensor->Pool();
	//}
}
