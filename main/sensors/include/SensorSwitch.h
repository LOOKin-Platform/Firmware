/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

#ifndef SENSORS_SWITCH_H
#define SENSORS_SWITCH_H

#include "Sensors.h"

class SensorSwitch_t : public Sensor_t {
	public:
		SensorSwitch_t();

		void		Update() override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;
		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;
};

#endif