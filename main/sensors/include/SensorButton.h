/*
*    SensorButton.cpp
*    SensorButton_t implementation
*
*/

#ifndef SENSORS_BUTTON_H
#define SENSORS_BUTTON_H

#include "Sensors.h"

class SensorButton_t : public Sensor_t {
	public:
		SensorButton_t();

		void		Update() override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;
		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;
};

#endif