/*
 *    Sensors.cpp
 *    Class to handle API /Sensors
 *
 */
#include "Globals.h"
#include "Sensors.h"
#include "Matter.h"

#include "SensorIR.h"
#include "SensorMultiSwitch.h"
#include "SensorSwitch.h"
#include "SensorButton.h"
#include "SensorMeteo.h"
#include "SensorWindowOpener.h"

Sensor_t::Sensor_t() {}

vector<Sensor_t*> Sensor_t::GetSensorsForDevice() {
	vector<Sensor_t*> Sensors = {};

	switch (Device.Type.Hex) {
#ifdef BUILD1GEN
		case Settings.Devices.Remote:
			Sensors = { new SensorIR_t() };
		break;
#else
		case Settings.Devices.Plug:
			Sensors = { 
				new SensorSwitch_t(),
				new SensorButton_t() 
			};
			break;
		case Settings.Devices.Duo:
			Sensors = {
				new SensorMultiSwitch_t(),
				new SensorButton_t()
			};
			break;

		case Settings.Devices.Remote:
			if (Settings.eFuse.Model < 2)
				Sensors = { new SensorIR_t() };
			else
				Sensors = { new SensorIR_t(), new SensorMeteo_t() };
		break;

		case Settings.Devices.WindowOpener:
			if (Settings.eFuse.Model == 0xF0)
				Sensors = { new SensorWindowOpener_t() };
			break;

		//case Settings.Devices.Motion:
		//	Sensors = { new SensorMotion_t() };
#endif
	}

	for (auto& SensorItem : Sensors)
		SensorItem->InitSettings();

	return Sensors;
}

Sensor_t* Sensor_t::GetSensorByName(string SensorName) {
	for (auto& Sensor : Sensors)
		if (Converter::ToLower(Sensor->Name) == Converter::ToLower(SensorName))
			return Sensor;

	return nullptr;
}

Sensor_t* Sensor_t::GetSensorByID(uint8_t SensorID) {
	for (auto& SensorItem : Sensors)
		if (SensorItem->ID == SensorID)
			return SensorItem;

	return nullptr;
}

uint8_t Sensor_t::GetDeviceTypeHex() {
	return Device.Type.Hex;
}

void Sensor_t::UpdateSensors() {
	if (Device.Status != RUNNING)
		return;

	for (auto& Sensor : Sensors)
		if (Sensor->ShouldUpdateInMainLoop())
			Sensor->Update();
}

void Sensor_t::HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) {
	if (Query.Type == QueryType::GET)
		Sensor_t::UpdateSensors();

	// Echo list of all sensors
	if (Query.GetURLPartsCount() == 1 && Query.Type == QueryType::GET) {
		vector<string> Vector = vector<string>();
		for (auto& Sensor : Sensors)
			Vector.push_back(Sensor->Name);

		Result.Body = JSON::CreateStringFromVector(Vector);
	}

	// Запрос значений конкретного сенсора
	if (Query.GetURLPartsCount() == 2) {
		Sensor_t* Sensor = Sensor_t::GetSensorByName(Query.GetStringURLPartByNumber(1));

		if (Sensor == nullptr) {
			Result.SetInvalid();
			return;
		}

		if (Query.Type == QueryType::GET)
			Result.Body = Sensor->RootSensorJSON();

		return;
	}

	// Запрос строковых значений состояния конкретного сенсора
	// Или получение JSON дополнительных значений
	if (Query.GetURLPartsCount() == 3) {
		if (Query.Type == QueryType::GET && !Query.CheckURLPart("settings", 2)) {
			for (Sensor_t* Sensor : Sensors)
				if (Query.CheckURLPart(Converter::ToLower(Sensor->Name),1)) {
					if (Converter::ToLower(Query.GetStringURLPartByNumber(2)) == "updated") {
						Result.Body = Converter::ToString(Sensor->Updated);
						return;
					}

					if (Sensor->Values.size() > 0)
						for (const auto &Value : Sensor->Values) {
							if (Query.CheckURLPart(Converter::ToLower(Value.first), 2)) {
								Result.Body = Sensor->FormatValue(Value.first);
								break;
							}
						}
				}
		}
		else if (Query.Type == QueryType::GET && Query.CheckURLPart("settings", 2)) {
			Sensor_t* Sensor = Sensor_t::GetSensorByName(Query.GetStringURLPartByNumber(1));

			if (Sensor == nullptr) {
				Result.SetInvalid();
				return;
			}

			Result.Body = Sensor->GetSettings();

			if (Result.Body == "")
				Result.SetInvalid();

			return;
		}
		else if (Query.Type == QueryType::PUT && Query.CheckURLPart("settings", 2)) {
			Sensor_t* Sensor = Sensor_t::GetSensorByName(Query.GetStringURLPartByNumber(1));

			if (Sensor == nullptr) {
				Result.SetInvalid();
				return;
			}

			Sensor->SetSettings(Result, Query);

			return;
		}

	}
}

// возвращаемое значение - было ли изменено значение в памяти
bool Sensor_t::SetValue(uint32_t Value, string Key, uint32_t UpdatedTime) {
	bool IsChanged = SetValue(Value, Key);

	if (IsChanged)
		Updated = (UpdatedTime > 0) ? UpdatedTime : ::Time::Unixtime();

	return IsChanged;
}

bool Sensor_t::SetValue(uint32_t Value, string Key) {
	bool IsChanged = true;

	if (Values.count(Key) > 0)
		if (Values[Key] == Value)
			IsChanged = false;

	if (!IsChanged) return false;

	Values[Key] = Value;

	return true;
}

string Sensor_t::RootSensorJSON() {
	if (Values.size() > 0) {
		JSON JSONObject;

		if (HasPrimaryValue())
			JSONObject.SetItem("Value", FormatValue());

		// Дополнительные значения сенсора, кроме Primary. Например - яркость каналов в RGBW Switch

		if (Values.size() > 0) {
			for (const auto &Value : Values) {
				ESP_LOGE("Value", "Key: %s, Value: %lu", Value.first.c_str(), Value.second);
				if (Value.first == "Primary")
					continue;

				if (ValueType(Value.first) == JSON::ValueType::Float)
					JSONObject.SetFloatItem(Value.first, Value.second);
				else if (ValueType(Value.first) == JSON::ValueType::Bool)
					JSONObject.SetBoolItem(Value.first, Value.second);
				else
					JSONObject.SetItem(Value.first, FormatValue(Value.first));
			}
		}

		JSONObject.SetItem("Updated", Converter::ToString(Updated));
		return JSONObject.ToString();
	}

	return "{}";
}

string Sensor_t::EchoSummaryJSON() {
	if (SummaryJSON() != "")
		return SummaryJSON();

	WebServer_t::Response Response;

	Query_t Query(("GET /sensors/" + Name).c_str());

	Sensor_t::HandleHTTPRequest	(Response, Query);

	return Response.Body;
}

uint32_t Sensor_t::GetValue(string Key) {
	if (Values.count(Key) == 0)
		return Settings.Memory.Empty32Bit;

	return Values[Key];
}

bool Sensor_t::IsMatterEnabled() 		{ return Matter::IsEnabledForDevice();		}

