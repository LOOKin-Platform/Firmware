/*
*    SensorTouch.cpp
*    SensorTouch_t implementation
*
*/


#ifndef SENSORS_TOUCH_H
#define SENSORS_TOUCH_H

#include "Sensors.h"

class SensorTouch_t : public Sensor_t {
	private:
		static inline map<gpio_num_t, uint64_t> 	SensorTouchStatusMap 	= {};
		static inline vector<gpio_num_t> 			SensorTouchGPIOS 		= vector<gpio_num_t>();
		static inline uint8_t 						SensorTouchID 			= 0xE2;

		static void SensorTouchTask(void *);

	public:
		static void Callback(void* arg);

		SensorTouch_t();
		
		void		Update() override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;
		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;
};

#endif