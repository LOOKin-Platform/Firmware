/*
*    SensorTemperatureRemote.cpp
*    SensorTemperatureRemote_t implementation
*
*/

#include "SensorMeteo.h"

#include "ds18b20-wrapper.h"
#include "BME280.h"
#include "hdc1080.h"

#include "Matter.h"
#include "Thermostat.h"
#include "HumiditySensor.h"
#include "TempSensor.h"
#include "DataRemote.h"
#include "PowerManagement.h"
#include "Wireless.h"
#include "Automation.h"

extern DataEndpoint_t 	*Data;
extern Wireless_t		Wireless;
extern Automation_t		Automation;

bool SensorMeteo_t::HasPrimaryValue() {
	return false;
}

bool SensorMeteo_t::ShouldUpdateInMainLoop() {
	return false;
}

SensorMeteo_t::SensorMeteo_t() {
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

string SensorMeteo_t::FormatValue(string Key) {
	if (Key == "Primary" || Key == "Temperature")
		return (Values[Key] >= 0x1000)
				? "-" + Converter::ToString(((float)(Values[Key]-0x1000) / 10))
				: Converter::ToString(((float)Values[Key] / 10));

	if (Key == "IsExternalSensorConnected")
		return (Values[Key] > 0) ? "1" : "0";

	return Converter::ToString(((float)Values[Key])/10);
}

void SensorMeteo_t::Pool() {
	if (Time::Uptime() %5 != 0) // read temp only in 5 seconds interval
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

uint32_t SensorMeteo_t::ValueToUint32(float Value, string Type) {
	uint32_t Result = 0;

	if (Type == "Temperature") {
		Result = (Value < 0) ? 0x1000 : 0x0000;

		Result	+= (uint32_t)abs(Value * 10);

		if (IsMatterEnabled() && Settings.eFuse.Type == Settings.Devices.Remote)
		{
			for (auto &IRDevice : ((DataRemote_t *)Data)->IRDevicesCache)
				if (IRDevice.DeviceType == 0xEF)
				{
					MatterThermostat* Thermostat = (MatterThermostat*)Matter::GetBridgedAccessoryByType(MatterGenericDevice::Thermostat, IRDevice.DeviceID);

					if (Thermostat != nullptr)
                        Thermostat->SetLocalTemperature(Value);;
                }

			MatterTempSensor* TempSensor = (MatterTempSensor*)Matter::GetBridgedAccessoryByType(MatterGenericDevice::Temperature);
			
			if (TempSensor != nullptr)
				TempSensor->SetTemperature(Value);
		}
	}
	else if (Type == "Humidity") {
		Result	+= (uint32_t)abs(Value * 10);

		if (IsMatterEnabled() && Settings.eFuse.Type == Settings.Devices.Remote)
		{
			MatterHumiditySensor* HumiditySensor = (MatterHumiditySensor*)Matter::GetBridgedAccessoryByType(MatterGenericDevice::Humidity);
			if (HumiditySensor != nullptr)
				HumiditySensor->SetHumidity(Value);
		}
	}
	else
		Result	+= (uint32_t)abs(Value * 10);

	return Result;
}

bool SensorMeteo_t::UpdateTempValue(float Temperature) {
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

bool SensorMeteo_t::UpdateHumidityValue(float Humidity) {
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

void SensorMeteo_t::UpdatePressureValue(float Pressure) {
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

SensorMeteo_t::SensorData SensorMeteo_t::GetValueFromSensor() {
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

	SensorMeteo_t::SensorData Result;
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
				if (PowerManagement::GetPMType() == PowerManagement::PowerManagementType::NONE)
				{
					float TempCorrection = 4.3;
					if (Time::Uptime() < 1800)
						TempCorrection = 1.015 * log10(Time::Uptime());

					Result.Temperature -= TempCorrection;
				}
			}
			else if (Settings.eFuse.Revision > 1)
			{
				hdc1080_read(HDC1080, &Result.Temperature, &Result.Humidity);

				if (Result.Temperature == 0 && Result.Humidity == 0) {
					Result.IsReceived = false;
					return Result;
				}

				if (PowerManagement::GetPMType() == PowerManagement::PowerManagementType::NONE)
				{
					float TempCorrection = 4.76; // 1.5
					if (Time::Uptime() < 1800)
						TempCorrection = 0.85 * log10(Time::Uptime());

					Result.Temperature -= TempCorrection;
				}
			}

			Result.Humidity += HumidityZeroCorrection;
			if (Result.Humidity < 0) Result.Humidity = 0;
			if (Result.Humidity > 100) Result.Humidity = 100;

			Result.Temperature += (ExternalTemp.size() > 0) ? ExternalTempCorrection : InternalTempCorrection;
		}
	}

	return Result;
}

void SensorMeteo_t::InitSettings() {
	NVS Memory(NVSSensorMeteoArea);

	if (Memory.IsKeyExists(NVSSensorMeteoInternalTempCorrection)) 	InternalTempCorrection = (float)Memory.GetInt16Bit(NVSSensorMeteoInternalTempCorrection) / 100;
	if (Memory.IsKeyExists(NVSSensorMeteoExternalTempCorrection)) 	ExternalTempCorrection = (float)Memory.GetInt16Bit(NVSSensorMeteoExternalTempCorrection) / 100;

	if (Memory.IsKeyExists(NVSSensorMeteoHumidityZeroCorrection))
		HumidityZeroCorrection = (float)Memory.GetInt16Bit(NVSSensorMeteoHumidityZeroCorrection) / 100;
	else
	{
		HumidityZeroCorrection = 15.2;
		Memory.SetInt16Bit(NVSSensorMeteoHumidityZeroCorrection, (int16_t)ceil(HumidityZeroCorrection * 100));
		Memory.Commit();
	}

	if (Memory.IsKeyExists(NVSSensorMeteoShowInExperimental))		SIEFlag = (Memory.GetInt8Bit(NVSSensorMeteoShowInExperimental) > 0) ? true : false;
}

string SensorMeteo_t::GetSettings() {
	return "{\"InternalTempCorrection\": " +
			Converter::FloatToString(InternalTempCorrection, 2) +
			", \"ExternalTempCorrection\": " +
			Converter::FloatToString(ExternalTempCorrection, 2) +
			", \"HumidityZeroCorrection\": " +
			Converter::FloatToString(HumidityZeroCorrection, 2) +
			", \"SIEFlag\": " + ((SIEFlag) ? "true" : "false") + "}";
}

bool SensorMeteo_t::GetSIEFlag() {
	return SIEFlag;
}

void SensorMeteo_t::SetSettings(WebServer_t::Response &Result, Query_t &Query) {
	JSON JSONItem(Query.GetBody());

	if (JSONItem.GetKeys().size() == 0)
	{
		Result.SetInvalid();
		return;
	}

	bool IsChanged = false;

	NVS Memory(NVSSensorMeteoArea);

	if (JSONItem.IsItemExists("InternalTempCorrection") && JSONItem.IsItemNumber("InternalTempCorrection"))
	{
		InternalTempCorrection = (float)ceil(JSONItem.GetDoubleItem("InternalTempCorrection") * 100.0) / 100.0;
		int16_t InternalTempCorrectionToSave = (int16_t)(InternalTempCorrection * 100);
		Memory.SetInt16Bit(NVSSensorMeteoInternalTempCorrection, InternalTempCorrectionToSave);
		IsChanged = true;
	}

	if (JSONItem.IsItemExists("ExternalTempCorrection") && JSONItem.IsItemNumber("ExternalTempCorrection"))
	{
		ExternalTempCorrection = (float)ceil(JSONItem.GetDoubleItem("ExternalTempCorrection") * 100.0) / 100.0;
		int16_t ExternalTempCorrectionToSave = (int16_t)(ExternalTempCorrection * 100);
		Memory.SetInt16Bit(NVSSensorMeteoExternalTempCorrection, ExternalTempCorrectionToSave);
		IsChanged = true;
	}

	if (JSONItem.IsItemExists("HumidityZeroCorrection") && JSONItem.IsItemNumber("HumidityZeroCorrection"))
	{
		HumidityZeroCorrection = (float)ceil(JSONItem.GetDoubleItem("HumidityZeroCorrection") * 100.0) / 100.0;
		int16_t HumidityZeroCorrectionToSave = (int16_t)(HumidityZeroCorrection * 100);
		Memory.SetInt16Bit(NVSSensorMeteoHumidityZeroCorrection, HumidityZeroCorrectionToSave);
		IsChanged = true;
	}

	if (JSONItem.IsItemExists("SIEFlag") && JSONItem.IsItemBool("SIEFlag"))
	{
		SIEFlag = JSONItem.GetBoolItem("SIEFlag");
		Memory.SetInt8Bit(NVSSensorMeteoShowInExperimental, (SIEFlag) ? 1 : 0);
		IsChanged = true;
	}

	if (IsChanged)
	{
		Memory.Commit();
		Result.SetSuccess();
	}
	else
		Result.SetInvalid();
}

uint32_t SensorMeteo_t::ReceiveValue(string Key) {
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

void SensorMeteo_t::Update() {
	Updated = Time::Unixtime();

	string NewStatus = Converter::ToHexString(GetValue("Temperature"),4) + Converter::ToHexString(GetValue("Humidity"),4);

	if (LastSendedStatus != NewStatus)
	{
		LastSendedStatus = NewStatus;
		Wireless.SendBroadcastUpdated(ID, "00", NewStatus, false);
		Automation.SensorChanged(ID);
	}
};

bool SensorMeteo_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
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

float SensorMeteo_t::ConvertToFloat(uint32_t Temperature)
{
	bool IsNegative = false;

	if (Temperature >= 0x1000)
		IsNegative = true;

	return (IsNegative) ? -((float)(Temperature - 0x1000) / 10) : (float)Temperature / 10;
}

float SensorMeteo_t::ConvertToFloat(uint8_t Temperature)
{
	return (Temperature >= 127) ? (Temperature - 127) : (127 - Temperature);
}
