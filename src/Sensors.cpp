/*
  Сенсоры - классы и функции, связанные с /sensors
*/

#include "Globals.h"
#include "Sensors.h"
#include "Device.h"
#include "Converter.h"
#include "JSON.h"

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
    if (Converter::ToLower(Sensor->Name) == Converter::ToLower(SensorName)) {
      return Sensor;
    }

  return new Sensor_t();
}

void Sensor_t::UpdateSensors() {
  for (auto& Sensor : Sensors)
    Sensor->Update();
}

void Sensor_t::HandleHTTPRequest(WebServerResponse_t* &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {

    Sensor_t::UpdateSensors();

    // Вывести список всех сенсоров
    if (URLParts.size() == 0) {
      JSON_t *JSON = new JSON_t();

      for (Sensor_t* Sensor : Sensors)
        JSON->AddToVector(Sensor->Name);

      Result->Body = JSON->ToString(true);
      delete JSON;
    }

    // Запрос значений конкретного сенсора
    if (URLParts.size() == 1) {

      Sensor_t* Sensor = Sensor_t::GetSensorByName(URLParts[0]);

      if (Sensor->Values.size() > 0) {
        JSON_t *JSON = new JSON_t();

        JSON->SetParam({
          {"Value"               , Sensor->Values["Primary"]["Value"]},
          {"Updated"             , Sensor->Values["Primary"]["Updated"]}
        });

        // Дополнительные значения сенсора, кроме Primary. Например - яркость каналов в RGBW Switch
        if (Sensor->Values.size() > 1) {
          for (const auto &Value : Sensor->Values)
            if (Value.first != "Primary") {
              map<string,string> SensorValue = Value.second;

              JSON->SetObjectParam(Value.first, {
                { "Value"   , SensorValue["Value"]},
                { "Updated" , SensorValue["Updated"]}
              });
            }
        }

        Result->Body = JSON->ToString();

        //delete Sensor;
        delete JSON;
      }
    }

    // Запрос строковых значений состояния конкретного сенсора
    // Или - получение JSON дополнительных значений
    if (URLParts.size() == 2)
      for (Sensor_t* Sensor : Sensors)
        if (Converter::ToLower(Sensor->Name) == URLParts[0]) {
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
              if (Converter::ToLower(Value.first) == URLParts[1]) {

                JSON_t *JSON = new JSON_t();

                map<string,string> SensorValue = Value.second;
                JSON->SetObjectParam(Value.first, {
                  { "Value"   , SensorValue["Value"]},
                  { "Updated" , SensorValue["Updated"]}
                });

                Result->Body = JSON->ToString();
                delete JSON;
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
              map<string, string> ValueItem = Value.second;

              if (URLParts[2] == "value")   Result->Body = ValueItem["Value"];
              if (URLParts[2] == "updated") Result->Body = ValueItem["Updated"];

              break;
            }
          }
}

/************************************/
/*          Switch Sensor           */
/************************************/

SensorSwitch_t::SensorSwitch_t() {
    ID          = 0x81;
    Name        = "Switch";
    EventCodes  = { 0x00, 0x01, 0x02 };
}

void SensorSwitch_t::Update() {
  string newValue = (GPIO::Read(SWITCH_PLUG_PIN_NUM) == true) ? "1" : "0";
  string oldValue = "";

  if (Values.count("Primary"))
    oldValue = Values["Primary"]["Value"];

  if (oldValue != newValue || Values.count("Primary") == 0) {
    Values["Primary"]["Value"]    = newValue;
    Values["Primary"]["Updated"]  = Time::UnixtimeString();

    Automation->SensorChanged(ID, (newValue == "1") ? 0x01 : 0x02 );
  }
}
