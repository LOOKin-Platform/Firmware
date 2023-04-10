/*
*    SensorSwitch.cpp
*    SensorSwitch_t implementation
*
*/

#ifndef SENSORS_MULTISWITCH_H
#define SENSORS_MULTISWITCH_H

class SensorMultiSwitch_t : public Sensor_t {
	private:
		vector<gpio_num_t> GPIOS = vector<gpio_num_t>();

	public:
		SensorMultiSwitch_t();

		void 		Update() override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;
		bool 		CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;
};
#endif
