#ifndef SENSOR_HANDLER
#define SENSOR_HANDLER

#include "Globals.h"

class SensorPeriodicHandler {
	public:
		static void Pool();
};

void IRAM_ATTR SensorPeriodicHandler::Pool() {
	//if (Time::Uptime() == 0) { // every 5 seconds pool sensors
		for (auto& Sensor : Sensors)
			Sensor->Pool();
	//}
}

#endif
