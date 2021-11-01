/*
*    SensorTemperatureRemote.cpp
*    SensorTemperatureRemote_t implementation
*
*/

#ifndef SENSORS_TEMPERATURE_REMOTE
#define SENSORS_TEMPERATURE_REMOTE

#include "ds18b20-wrapper.h"
#include "BME280.h"
#include "hdc1080.h"

#define SDA_PIN GPIO_NUM_18
#define SCL_PIN GPIO_NUM_19

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
	public:
		struct SensorData {
			float 	Temperature;
			float 	Humidity;
			float 	Pressure;
			bool	IsReceived = true;
		};

		bool HasPrimaryValue() override {
			return false;
		}

		bool ShouldUpdateInMainLoop() override {
			return false;
		}

		SensorMeteo_t() {
			if (GetIsInited()) return;

			ID          = 0xFE;
			Name        = "Meteo";
			EventCodes  = { 0x00, 0x02, 0x03, 0x05, 0x06  };

			SetValue(0, "Temperature");
			SetValue(0, "Humidity");
			SetValue(0, "Pressure");
			SetValue(0, "IsExternalSensorConnected");

			SetIsInited(true);
		}

		string FormatValue(string Key = "Primary") override {
			if (Key == "Primary" || Key == "Temperature")
				return (Values[Key] >= 0x1000)
						? "-" + Converter::ToString(((float)(Values[Key]-0x1000) / 10))
						: Converter::ToString(((float)Values[Key] / 10));

			if (Key == "IsExternalSensorConnected")
				return (Values[Key] > 0) ? "1" : "0";

			return Converter::ToString(((float)Values[Key])/10);
		}


		void Pool() override {
			if (Time::Uptime() %3 != 0) // read temp only in 3 seconds interval
				return;

			if (Settings.eFuse.Type != Settings.Devices.Remote)
				return;

			if (Settings.eFuse.Model < 2)
				return;

			SensorData CurrentData = GetValueFromSensor();

			if (!CurrentData.IsReceived)
				return;

			bool IsTempChanged 		= UpdateTempValue(CurrentData.Temperature);
			bool IsHumidityChanged	= UpdateHumidityValue(CurrentData.Humidity);
			UpdatePressureValue(CurrentData.Pressure);

			if (IsTempChanged || IsHumidityChanged)
				Update();
		}

		uint32_t ValueToUint32(float Value, string Type) {
			uint32_t Result = 0;

			if (Type == "Temperature") {
				Result = (Value < 0) ? 0x1000 : 0x0000;

				Result	+= (uint32_t)abs(Value * 10);

				if (IsHomeKitEnabled() && Settings.eFuse.Type == Settings.Devices.Remote)
				{
					bool IsEmptyAC = true;

					hap_val_t CurrentTempValue;
					CurrentTempValue.f = Value;

					for (auto &IRDevice : ((DataRemote_t *)Data)->IRDevicesCache)
						if (IRDevice.DeviceType == 0xEF)
						{
							HomeKitUpdateCharValue(IRDevice.DeviceID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_TEMPERATURE, CurrentTempValue);
							IsEmptyAC = false;
						}

					if (IsEmptyAC)
						HomeKitUpdateCharValue(0, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_TEMPERATURE, CurrentTempValue);
				}
			}
			else if (Type == "Humidity") {
				Result	+= (uint32_t)abs(Value * 10);

				if (IsHomeKitEnabled() && Settings.eFuse.Type == Settings.Devices.Remote)
				{
					bool IsEmptyAC = true;

					hap_val_t CurrentHumidityValue;
					CurrentHumidityValue.f = Value;

					for (auto &IRDevice : ((DataRemote_t *)Data)->IRDevicesCache)
						if (IRDevice.DeviceType == 0xEF)
						{
							HomeKitUpdateCharValue(IRDevice.DeviceID, HAP_SERV_UUID_HUMIDITY_SENSOR, HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY, CurrentHumidityValue);
							IsEmptyAC = false;
						}

					if (IsEmptyAC)
						HomeKitUpdateCharValue(0, HAP_SERV_UUID_HUMIDITY_SENSOR, HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY, CurrentHumidityValue);
				}
			}
			else
				Result	+= (uint32_t)abs(Value * 10);

			return Result;
		}

		bool UpdateTempValue(float Temperature) {
			uint32_t 	NewValue 	= ValueToUint32(Temperature, "Temperature");
			uint32_t	OldValue	= GetValue("Temperature");

			bool ShouldUpdate = true;

			if (OldValue != numeric_limits<uint32_t>::max()) {
				uint32_t DeltaTemp = (uint32_t)abs((int64_t)NewValue - (int64_t)OldValue);
				uint32_t DeltaTime = Time::Uptime() - PreviousTempTime;

				if (DeltaTemp < 2 && DeltaTime < 60)
					ShouldUpdate = false;
			}

			if (ShouldUpdate) {
				PreviousTempTime 	= Time::Uptime();
				PreviousTempValue 	= OldValue;
				SetValue(NewValue, "Temperature");
				return true;
			}

			return false;
		}

		bool UpdateHumidityValue(float Humidity) {
			uint32_t 	NewValue 	= ValueToUint32(Humidity, "Humidity");
			uint32_t	OldValue	= GetValue("Humidity");

			bool ShouldUpdate = true;

			if (OldValue != numeric_limits<uint32_t>::max()) {
				uint32_t DeltaHumidity 	= (uint32_t)abs((int64_t)NewValue - (int64_t)OldValue);
				uint32_t DeltaTime 		= Time::Uptime() - PreviousHumidityTime;

				if (DeltaHumidity < 4 && DeltaTime < 60)
					ShouldUpdate = false;
			}

			if (ShouldUpdate) {
				PreviousHumidityTime	= Time::Uptime();
				PreviousHumidityValue 	= OldValue;
				SetValue(NewValue, "Humidity");
				return true;
			}

			return false;
		}

		void UpdatePressureValue(float Pressure) {
			uint32_t 	NewValue 	= ValueToUint32(Pressure, "Pressure");
			uint32_t	OldValue	= GetValue("Pressure");

			bool ShouldUpdate = true;

			if (OldValue != numeric_limits<uint32_t>::max()) {
				uint32_t Delta 		= (uint32_t)abs((int64_t)NewValue - (int64_t)OldValue);
				uint32_t DeltaTime 	= Time::Uptime() - PreviousPressureTime;

				if (Delta < 50 && DeltaTime < 60)
					ShouldUpdate = false;
			}

			if (ShouldUpdate) {
				PreviousPressureTime	= Time::Uptime();
				PreviousPressureValue 	= OldValue;
				SetValue(NewValue, "Pressure");
			}
		}

		SensorData GetValueFromSensor() {
			//Settings.eFuse.Revision = 2;
			if (!IsSensorHWInited) // Init sensors
			{
				if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1)
					DS18B20::Init(GPIO_NUM_15);

				if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1 && Settings.eFuse.Revision == 1)
				{
				    bme280.setDebug(false);
				    bme280.init();
				}
				else if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1 && Settings.eFuse.Revision > 1)
				{
				    i2c_init(0, GPIO_NUM_19, GPIO_NUM_18, I2C_FREQ_100K);

					HDC1080 = hdc1080_init_sensor(0, HDC1080_ADDR);

					hdc1080_registers_t registers = hdc1080_get_registers(HDC1080);
			        registers.acquisition_mode = 1;
			        hdc1080_set_registers(HDC1080, registers);
				}

			    IsSensorHWInited = true;
			}

			SensorData Result;
			Result.Temperature = 0;
			Result.Humidity = 0;
			Result.Pressure = 0;
			Result.IsReceived = true;

			if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1)
			{
				vector<float> ExternalTemp = DS18B20::ReadData();

				if (ExternalTemp.size() > 0) {
					SetValue(1, "IsExternalSensorConnected");
					Result.Temperature = ExternalTemp.at(0);
				}
				else
				{
					SetValue(0, "IsExternalSensorConnected");

					if (Settings.eFuse.Revision == 1)
					{
						bme280_reading_data sensor_data = bme280.readSensorData();

						Result.Temperature 	= sensor_data.temperature;
						Result.Humidity		= sensor_data.humidity;
						Result.Pressure		= sensor_data.pressure;

						if (Result.Temperature == 0 && Result.Humidity == 0) {
							Result.IsReceived = false;
							return Result;
						}

						// корректировка значения BME280 (без учета времени старта, нахолодную)
						float TempCorrection = 3.3;
						if (Time::Uptime() < 1800)
							TempCorrection = 1.015 * log10(Time::Uptime());

						Result.Temperature -= TempCorrection;

					}
					else if (Settings.eFuse.Revision > 1)
					{
						hdc1080_read(HDC1080, &Result.Temperature, &Result.Humidity);

						if (Result.Temperature == 0 && Result.Humidity == 0) {
							Result.IsReceived = false;
							return Result;
						}

						float TempCorrection = 2.76;
						if (Time::Uptime() < 1800)
							TempCorrection = 0.85 * log10(Time::Uptime());

						Result.Temperature -= TempCorrection;
					}

					Result.Humidity += 15.2;
					if (Result.Humidity > 100)
						Result.Humidity = 100;
				}
			}

			return Result;
		}

		uint32_t ReceiveValue(string Key = "Primary") override {
			if (!IsSensorHWInited)
			{
				SensorData Value = GetValueFromSensor();

				if (!Value.IsReceived)
					return 0;

				if (Key == "Temperature")
					return ValueToUint32(Value.Temperature, "Temperature");
				else if (Key == "Humidity")
					return ValueToUint32(Value.Humidity, "Humidity");
				else if (Key == "Pressure")
					return ValueToUint32(Value.Pressure, "Pressure");
				else
					return 0;
			}

			return GetValue(Key);
		};

		void Update() override {
			Updated = Time::Unixtime();

			string NewStatus = Converter::ToHexString(GetValue("Temperature"),4) + Converter::ToHexString(GetValue("Humidity"),4);

			if (LastSendedStatus != NewStatus)
			{
				LastSendedStatus = NewStatus;
				Wireless.SendBroadcastUpdated(ID, "00", NewStatus, false);
				Automation.SensorChanged(ID);
			}
		};

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
			if (SceneEventCode == 0x02 || SceneEventCode == 0x03) {
				float Previous 				= ConvertToFloat(PreviousTempValue);
				float Current				= ConvertToFloat(GetValue("Temperature"));
				float Operand				= ConvertToFloat(SceneEventOperand);

				if (SceneEventCode == 0x02 && Previous <= Operand && Current > Operand)
					return true;

				if (SceneEventCode == 0x03 && Previous >= Operand && Current < Operand)
					return true;
			}

			if (SceneEventCode == 0x05 || SceneEventCode == 0x06) {
				float Previous 				= ConvertToFloat(PreviousHumidityValue);
				float Current				= ConvertToFloat(GetValue("Humidity"));
				float Operand				= ConvertToFloat(SceneEventOperand);

				if (SceneEventCode == 0x05 && Previous <= Operand && Current > Operand)
					return true;

				if (SceneEventCode == 0x06 && Previous >= Operand && Current < Operand)
					return true;
			}

			return false;
		}

		float ConvertToFloat(uint32_t Temperature)
		{
			bool IsNegative = false;

			if (Temperature >= 0x1000)
				IsNegative = true;

			return (IsNegative) ? -((float)(Temperature - 0x1000) / 10) : (float)Temperature / 10;
		}

		float ConvertToFloat(uint8_t Temperature)
		{
			return (Temperature >= 127) ? (Temperature - 127) : (127 - Temperature);
		}

};

#endif
