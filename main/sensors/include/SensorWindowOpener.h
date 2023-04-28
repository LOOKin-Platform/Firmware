/*
*    SensorWindowOpener.cpp
*    SensorWindowOpener_t implementation
*
*/

#ifndef SENSORS_WINDOWOPENER_H
#define SENSORS_WINDOWOPENER_H

#include "Sensors.h"

class SensorWindowOpener_t : public Sensor_t {
	public:
		SensorWindowOpener_t();

		bool HasPrimaryValue() override;
		bool ShouldUpdateInMainLoop() override;

		JSON::ValueType ValueType(string Key) override;
		string 			FormatValue(string Key) override;

		void 			Update() override;

		uint32_t 		ReceiveValue(string Key = "Primary") override;
		bool 			CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;
};

#endif