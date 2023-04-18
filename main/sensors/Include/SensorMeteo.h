/*
*    SensorTemperatureRemote.cpp
*    SensorTemperatureRemote_t implementation
*
*/

#ifndef SENSORS_METEO_REMOTE
#define SENSORS_METEO_REMOTE

#include "ds18b20-wrapper.h"
#include "BME280.h"
#include "hdc1080.h"

#include "Sensors.h"

#include "Matter.h"
#include "Thermostat.h"
#include "HumiditySensor.h"
#include "TempSensor.h"

#define SDA_PIN GPIO_NUM_18
#define SCL_PIN GPIO_NUM_19

#define	NVSSensorMeteoArea 						"SensorMeteo"
#define NVSSensorMeteoInternalTempCorrection	"IntTCorrection"
#define NVSSensorMeteoExternalTempCorrection	"ExtTCorrection"
#define NVSSensorMeteoHumidityZeroCorrection	"HumidityCorrect"
#define NVSSensorMeteoShowInExperimental		"SIEflag"

class SensorMeteo_t : public Sensor_t {
	private:
		BME280 				bme280			= 0;
		hdc1080_sensor_t* 	HDC1080			= NULL;

		bool		IsSensorHWInited		= false;

		uint32_t	Value					= 0x0000;

		uint32_t	PreviousTempTime		= 0;
		uint32_t 	PreviousTempValue 		= numeric_limits<uint32_t>::max();

		uint32_t	PreviousHumidityTime	= 0;
		uint32_t 	PreviousHumidityValue 	= numeric_limits<uint32_t>::max();

		uint32_t	PreviousPressureTime	= 0;
		uint32_t 	PreviousPressureValue 	= numeric_limits<uint32_t>::max();

		string 		LastSendedStatus		= "";

		float 		InternalTempCorrection	= 0;
		float 		ExternalTempCorrection	= 0;
		float 		HumidityZeroCorrection	= 0;

		bool		SIEFlag					= true;

	public:
		struct SensorData {
			float 	Temperature;
			float 	Humidity;
			float 	Pressure;
			bool	IsReceived = true;
		};

		bool HasPrimaryValue() override;
		bool ShouldUpdateInMainLoop() override;
		
		SensorMeteo_t();

		string 		FormatValue(string Key = "Primary") override;
		
		void 		Pool() override;

		uint32_t 	ValueToUint32(float Value, string Type);

		bool 		UpdateTempValue(float Temperature);
		bool 		UpdateHumidityValue(float Humidity);
		void 		UpdatePressureValue(float Pressure);

		SensorData 	GetValueFromSensor();

		void 		InitSettings() override;
		string 		GetSettings() override;

		bool 		GetSIEFlag();
		void 		SetSettings(WebServer_t::Response &Result, Query_t &Query) override;
		uint32_t 	ReceiveValue(string Key = "Primary") override;

		void Update() override;
		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;

		float ConvertToFloat(uint32_t 	Temperature);
		float ConvertToFloat(uint8_t 	Temperature);

};

#endif
