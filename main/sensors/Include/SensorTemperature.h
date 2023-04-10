/*
*    SensorTemperature.cpp
*    SensorTemperature_t implementation
*
*/

#ifndef SENSORS_TEMPERATURE_H
#define SENSORS_TEMPERATURE_H

class SensorTemperature_t : public Sensor_t {
	protected:
		uint32_t 	PreviousValue 	= numeric_limits<uint32_t>::max();

	public:
		SensorTemperature_t();

		void 	Update() override;
		string 	FormatValue(string Key = "Primary") override;

		bool 	CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;

		string 	SummaryJSON() override;

		float 	ConvertToFloat(uint32_t Temperature);
		float 	ConvertToFloat(uint8_t Temperature);
};

#endif