/*
*    SensorButton.cpp
*    SensorButton_t implementation
*
*/


#ifndef SENSORS_BUTTON_H
#define SENSORS_BUTTON_H

#include "Sensors.h"

class SensorButton_t : public Sensor_t {
	private:
		static inline map<gpio_num_t, uint64_t> 	SensorButtonStatusMap 	= {};
		static inline vector<gpio_num_t> 			SensorButtonGPIOS 		= vector<gpio_num_t>();
		static inline uint8_t 						SensorButtonID 			= 0xE2;

		static void SensorButtonTask(void *);

	public:
		static void Callback(void* arg);
		static void SendToSensorTask(void *Status);

		SensorButton_t();
		
		void		Update() override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;
		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;
};

#endif