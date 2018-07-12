/*
 *    Sensors.cpp
 *    Class to handle API /Sensors
 *
 */
#include "Globals.h"
#include "Sensors.h"

Sensor_t::Sensor_t() {
}

vector<Sensor_t*> Sensor_t::GetSensorsForDevice() {
	vector<Sensor_t*> Sensors = {};

	switch (Device.Type.Hex) {
		case Settings.Devices.Plug:
			Sensors = { new SensorSwitch_t(), new SensorColor_t() };
			break;
		case Settings.Devices.Remote:
			Sensors = { new SensorIR_t(),  new SensorColor_t() };
			break;
		case Settings.Devices.Motion:
			Sensors = { new SensorMotion_t() };
			break;
	}

	return Sensors;
}

Sensor_t* Sensor_t::GetSensorByName(string SensorName) {
	for (auto& Sensor : Sensors)
		if (Converter::ToLower(Sensor->Name) == Converter::ToLower(SensorName)) {
			return Sensor;
		}

	return nullptr;
}

Sensor_t* Sensor_t::GetSensorByID(uint8_t SensorID) {
	for (auto& Sensor : Sensors)
		if (Sensor->ID == SensorID)
			return Sensor;

	return new Sensor_t();
}

uint8_t Sensor_t::GetDeviceTypeHex() {
	return Device.Type.Hex;
}


void Sensor_t::UpdateSensors() {
	for (auto& Sensor : Sensors)
		Sensor->Update();
}

void Sensor_t::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
	Sensor_t::UpdateSensors();

	// Echo list of all sensors
	if (URLParts.size() == 0) {
		vector<string> Vector = vector<string>();
		for (auto& Sensor : Sensors)
			Vector.push_back(Sensor->Name);

		Result.Body = JSON::CreateStringFromVector(Vector);
	}

	// Запрос значений конкретного сенсора
	if (URLParts.size() == 1) {
		Sensor_t* Sensor = Sensor_t::GetSensorByName(URLParts[0]);

		if (Sensor == nullptr) {
			Result.SetInvalid();
			return;
		}

		if (Sensor->Values.size() > 0) {

			JSON JSONObject;

			JSONObject.SetItems(vector<pair<string,string>>({
				make_pair("Value"	, Sensor->FormatValue()),
						make_pair("Updated"	, Converter::ToString(Sensor->Values["Primary"].Updated))
			}));

			// Дополнительные значения сенсора, кроме Primary. Например - яркость каналов в RGBW Switch
			if (Sensor->Values.size() > 1) {
				for (const auto &Value : Sensor->Values)
					if (Value.first != "Primary") {
						SensorValueItem SensorValue = Value.second;

						JSONObject.SetObject(Value.first,
						{
								{ "Value"   , Sensor->FormatValue(Value.first)},
								{ "Updated" , Converter::ToString(SensorValue.Updated)}
						});
					}
			}

			Result.Body = JSONObject.ToString();
		}
	}

	// Запрос строковых значений состояния конкретного сенсора
	// Или - получение JSON дополнительных значений
	if (URLParts.size() == 2)
		for (Sensor_t* Sensor : Sensors)
			if (Converter::ToLower(Sensor->Name) == URLParts[0]) {
				if (URLParts[1] == "value") {
					Result.Body = Sensor->FormatValue();
					break;
				}

				if (URLParts[1] == "updated") {
					Result.Body = Converter::ToString(Sensor->Values["Primary"].Updated);
					break;
				}

				if (Sensor->Values.size() > 0)
					for (const auto &Value : Sensor->Values) {
						if (Converter::ToLower(Value.first) == URLParts[1]) {
							JSON JSONObject;

							JSONObject.SetObject(Value.first, {
									{ "Value"   , Sensor->FormatValue(Value.first) },
									{ "Updated" , Converter::ToString(Value.second.Updated)}
							});

							Result.Body = JSONObject.ToString();
							break;
						}
						break;
					}
			}

	// Запрос строковых значений состояния дополнительного поля конкретного сенсора
	if (URLParts.size() == 3)
		for (Sensor_t* Sensor : Sensors)
			if (Converter::ToLower(Sensor->Name) == URLParts[0])
				for (const auto &Value : Sensor->Values) {
					string ValueItemName = Value.first;

					if (ValueItemName == URLParts[1])
					{
						if (URLParts[2] == "value")   Result.Body = Sensor->FormatValue(Value.first);
						if (URLParts[2] == "updated") Result.Body = Converter::ToString(Value.second.Updated);

						break;
					}
				}
}

// возвращаемое значение - было ли изменено значение в памяти
bool Sensor_t::SetValue(uint32_t Value, string Key) {
	if (Values.count(Key) > 0) {
		if (Values[Key].Value != Value) {
			Values[Key] = SensorValueItem(Value, Time::Unixtime());
			return true;
		}
	}
	else {
		Values[Key] = SensorValueItem(Value, Time::Unixtime());
		return true;
	}

	return false;
}

SensorValueItem Sensor_t::GetValue(string Key) {
	if (!(Values.count(Key) > 0))
		SetValue(ReceiveValue(Key));

	return Values[Key];
}
