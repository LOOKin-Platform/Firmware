/*
*    SensorColor.cpp
*    SensorColor_t implementation
*
*/

#ifndef SENSORS_RGBW_H
#define SENSORS_RGBW_H

#include "Sensor.h"

class SensorColor_t : public Sensor_t {
	public:
		SensorColor_t();
		void 		Update() override;

		uint32_t 	ReceiveValue(string Key = "Primary") override;

		string 		FormatValue(string Key) override; 

		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;

		uint8_t 	ToBrightness(uint32_t Color);
		uint8_t 	ToBrightness(uint8_t Red, uint8_t Green, uint8_t Blue);
};

#endif