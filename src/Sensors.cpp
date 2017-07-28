/*
  Сенсоры - классы и функции, связанные с /sensors
*/

#include "Globals.h"
#include "Sensors.h"
#include "Device.h"
#include "Tools.h"

#include "drivers/GPIO/GPIO.h"
#include "drivers/Time/Time.h"

//static char tag[] = "Sensors";

vector<Sensor_t*> Sensor_t::GetSensorsForDevice() {

  vector<Sensor_t*> Sensors = {};

  switch (Device->Type->Hex) {
    case DEVICE_TYPE_PLUG_HEX:
      Sensors = { new SensorSwitch_t() };
      break;
  }

  return Sensors;
}

Sensor_t* Sensor_t::GetSensorByName(string SensorName) {

  for (Sensor_t* Sensor : Sensors)
    if (Tools::ToLower(Sensor->Name) == Tools::ToLower(SensorName)) {
      return Sensor;
    }

  return new Sensor_t();
}

void Sensor_t::UpdateSensors() {
  for (auto Sensor : Sensors)
    Sensor->Update();
}

WebServerResponse_t* Sensor_t::HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params) {

    Sensor_t::UpdateSensors();

    WebServerResponse_t *Result = new WebServerResponse_t();

    // Вывести список всех сенсоров
    if (URLParts.size() == 0) {
      StringBuffer sb;
      Writer<StringBuffer> Writer(sb);

      vector<string> *JSONVector = new vector<string>();
      for (Sensor_t* Sensor : Sensors)
        JSONVector->push_back(Sensor->Name);

      JSON_t::CreateArrayFromVector(*JSONVector, Writer);

      Result->Body = string(sb.GetString());
    }

    // Запрос значений конкретного сенсора
    if (URLParts.size() == 1) {

      Sensor_t* Sensor = Sensor_t::GetSensorByName(URLParts[0]);

      if (Sensor->Values.size() > 0) {

        StringBuffer sb;
        Writer<StringBuffer> Writer(sb);

        Sensor_t::PrepareValues(Sensor->Values["Primary"], Writer);

        // Дополнительные значения сенсора, кроме Primary. Например - яркость каналов в RGBW Switch
        if (Sensor->Values.size() > 1) {
          for (const auto &Value : Sensor->Values)
            if (Value.first != "Primary") {
              Writer.StartObject();
              Writer.Key(Value.first.c_str());
              Sensor_t::PrepareValues(Value.second, Writer);
              Writer.EndObject();
            }
        }

        Result->Body = string(sb.GetString());
      }

    }

    // Запрос строковых значений состояния конкретного сенсора
    // Или - получение JSON дополнительных значений
    if (URLParts.size() == 2)
      for (Sensor_t* Sensor : Sensors)
        if (Tools::ToLower(Sensor->Name) == URLParts[0]) {
          if (URLParts[1] == "value") {
            Result->Body = Sensor->Values["Primary"]["Value"];
            break;
          }

          if (URLParts[1] == "updated") {
            Result->Body = Sensor->Values["Primary"]["Updated"];
            break;
          }

          if (Sensor->Values.size() > 0)
            for (const auto &Value : Sensor->Values) {
              if (Tools::ToLower(Value.first) == URLParts[1]) {

                StringBuffer sb;
                Writer<StringBuffer> Writer(sb);
                Sensor_t::PrepareValues(Value.second, Writer);

                Result->Body = string(sb.GetString());
                break;
              }
          break;
      }
    }

    // Запрос строковых значений состояния дополнительного поля конкретного сенсора
    if (URLParts.size() == 3)
      for (Sensor_t* Sensor : Sensors)
        if (Tools::ToLower(Sensor->Name) == URLParts[0])
          for (const auto &Value : Sensor->Values) {
            string ValueItemName = Value.first;

            if (ValueItemName == URLParts[1])
            {
              map<string, string> ValueItem = Value.second;

              if (URLParts[2] == "value")   Result->Body = ValueItem["Value"];
              if (URLParts[2] == "updated") Result->Body = ValueItem["Updated"];

              break;
            }
          }

    return Result;
}

void Sensor_t::PrepareValues(map<string, string> Values, Writer<StringBuffer> &Writer) {

  map<string,string> JSONMap = {
    {"Value"               , Values["Value"]},
    {"Updated"             , Values["Updated"]}
  };

  JSON_t::CreateObjectFromMap(JSONMap, Writer);

  return;
}

/************************************/
/*          Switch Sensor           */
/************************************/

SensorSwitch_t::SensorSwitch_t() {
    ID          = "10000001";
    Name        = "Switch";
    EventCodes  = { "0000", "0001", "0010" };
}

void SensorSwitch_t::Update() {
  string Value = (GPIO::Read(SWITCH_PLUG_PIN_NUM) == true) ? "1" : "0";

   map<string,map<string, string>>::iterator It = Values.find("Primary");

  if (It != Values.end())
  {
    if (Values["Primary"]["Value"] != Value)
    {
      Values["Primary"]["Value"] = Value;
      Values["Primary"]["Updated"] = Time::GetTimeString();
    }
  }
  else
  {
    Values["Primary"]["Value"] = Value;
    Values["Primary"]["Updated"] = Time::GetTimeString();
  }

}
