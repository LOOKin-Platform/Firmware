/*
*    SensorTemperatureRemote.cpp
*    SensorTemperatureRemote_t implementation
*
*/

#ifndef SENSORS_TEMPERATURE_REMOTE
#define SENSORS_TEMPERATURE_REMOTE

#include "BME280.h"

#define SDA_PIN GPIO_NUM_18
#define SCL_PIN GPIO_NUM_19

class SensorTemperatureRemote_t : public SensorTemperature_t {
	private:
		BME280 		bme280			= 0;
		uint32_t 	PreviousValue 	= 0;
		bool		IsSensorHWInited= false;

		uint32_t	Value			= 0x0000;

	public:
		void Pool() override {
			if (Time::Uptime() %2 != 0) // read temp only in 2 seconds interval
				return;

			if (Settings.eFuse.Type != Settings.Devices.Remote)
				return;

			if (Settings.eFuse.Model < 2)
				return;

			uint32_t 	NewValue 	= GetValueFromSensor();
			uint32_t	OldValue	= GetValue().Value;
			if (NewValue != OldValue) {
				PreviousValue = OldValue;
				SetValue(NewValue);
				Update();
			}
		}

		uint32_t GetValueFromSensor() {
			if (!IsSensorHWInited) {
			    bme280.setDebug(false);
			    bme280.init();
			    IsSensorHWInited = true;
			}

			bme280_reading_data sensor_data = bme280.readSensorData();
			/*
			printf("Temperature: %.2foC, Humidity: %.2f%%, Pressure: %.2fPa\n",
		               (double) sensor_data.temperature,
		               (double) sensor_data.humidity,
		               (double) sensor_data.pressure
		        );
			*/
			float Temperature = (float) sensor_data.temperature;
			uint32_t Value = (Temperature < 0) ? 0x1000 : 0x0000;

			Value	+= (uint32_t)abs(Temperature * 10);

#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
			// Update HomeKit AC temp values
			for (auto &IRDevice : ((DataRemote_t *)Data)->GetAvaliableDevices())
				if (IRDevice.Type == 0xEF) {
					hap_val_t CurrentTempValue;
					CurrentTempValue.f = Temperature;
					HomeKitUpdateCharValue(IRDevice.UUID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_TEMPERATURE, CurrentTempValue);
				}
#endif

			return Value;
		}

		uint32_t ReceiveValue(string Key = "Primary") override {
			if (!IsSensorHWInited)
				return GetValueFromSensor();

			return GetValue().Value;
		};
};

#endif
