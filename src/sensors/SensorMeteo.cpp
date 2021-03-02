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

class SensorMeteo_t : public Sensor_t {
	private:
		BME280 		bme280			= 0;
		bool		IsSensorHWInited= false;

		uint32_t	Value			= 0x0000;

		uint32_t	PreviousTempTime		= 0;
		uint32_t 	PreviousTempValue 		= numeric_limits<uint32_t>::max();

		uint32_t	PreviousHumidityTime	= 0;
		uint32_t 	PreviousHumidityValue 	= numeric_limits<uint32_t>::max();

		uint32_t	PreviousPressureTime	= 0;
		uint32_t 	PreviousPressureValue 	= numeric_limits<uint32_t>::max();

		string 		LastSendedStatus		= "";
	public:
		struct SensorData {
			float Temperature;
			float Humidity;
			float Pressure;
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

			SetIsInited(true);
		}

		string FormatValue(string Key = "Primary") override {
			if (Key == "Primary" || Key == "Temperature")
				return (Values[Key] >= 0x1000)
						? "-" + Converter::ToString(((float)(Values[Key]-0x1000) / 10))
						: Converter::ToString(((float)Values[Key] / 10));

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

				if (IsHomeKitEnabled()) {
					for (auto &IRDevice : ((DataRemote_t *)Data)->IRDevicesCache)
						if (IRDevice.DeviceType == 0xEF) {
							hap_val_t CurrentTempValue;
							CurrentTempValue.f = Value;
							HomeKitUpdateCharValue(IRDevice.DeviceID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_TEMPERATURE, CurrentTempValue);
						}
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
			if (!IsSensorHWInited) {
			    bme280.setDebug(false);
			    bme280.init();
			    IsSensorHWInited = true;
			}

			bme280_reading_data sensor_data = bme280.readSensorData();

			//printf("Temperature: %.2foC, Humidity: %.2f%%, Pressure: %.2fPa\n", (double)sensor_data.temperature, (double)sensor_data.humidity, (double)sensor_data.pressure);

			SensorData Result;
			Result.Temperature 	= sensor_data.temperature;
			Result.Humidity		= sensor_data.humidity;
			Result.Pressure		= sensor_data.pressure;

			// корректировка значения BME280 (без учета времени старта, нахолодную)
			float TempCorrection = 3;
			if (Time::Uptime() < 1800)
				TempCorrection = 0.9 * log10(Time::Uptime() + 1);

			Result.Temperature -= TempCorrection;

			return Result;
		}

		uint32_t ReceiveValue(string Key = "Primary") override {
			if (!IsSensorHWInited) {
				SensorData Value = GetValueFromSensor();

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
