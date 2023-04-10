/*
*    SensorMotion.cpp
*    SensorMotion_t implementation
*
*/

#ifndef SENSORS_MOTION_H
#define SENSORS_MOTION_H

class SensorMotion_t : public Sensor_t {
	public:
		SensorMotion_t();

		void 		Update() override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;
		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;

	private:
		static void MotionDetectTask(void *);
		
		static inline uint8_t 			SensorMotionID 		= 0xE1;
		static inline adc1_channel_t 	SensorMotionChannel	= ADC1_CHANNEL_0;

		static inline const adc_atten_t	atten = ADC_ATTEN_11db;
};
#endif
