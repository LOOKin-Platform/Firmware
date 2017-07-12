/*
  Сенсоры - классы и функции, связанные с /sensors
*/

#include "include/Globals.h"
#include "include/Sensors.h"
#include "include/Device.h"
#include "include/Tools.h"

#include <cJSON.h>

#include "drivers/GPIO/GPIO.h"
#include "drivers/Time/Time.h"

vector<Sensor_t> Sensor_t::GetSensorsForDevice() {

  vector<Sensor_t> Sensors = {};

  switch (Device->Type) {
    case DeviceType::PLUG:
      Sensors = { SensorSwitch_t() };
      break;
  }

  return Sensors;
}

void Sensor_t::UpdateSensors() {
  for (auto& Sensor : Sensors)
    Sensor.Update();
}

WebServerResponse_t* Sensor_t::HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params) {
    Sensor_t::UpdateSensors();

    WebServerResponse_t *Result = new WebServerResponse_t();
    cJSON *Root, *Helper;

    // Вывести список всех сенсоров
    if (URLParts.size() == 0) {
      // Подготовка списка сенсоров для вывода
      vector<const char *> SensorNames;
      for (Sensor_t& Sensor : Sensors)
        SensorNames.push_back(Sensor.Name.c_str());

      Root = cJSON_CreateStringArray(SensorNames.data(), SensorNames.size());

      Result->Body = string(cJSON_Print(Root));
    }

    // Запрос значений конкретного сенсора
    if (URLParts.size() == 1) {
      for (Sensor_t& Sensor : Sensors)
        if (Tools::ToLower(Sensor.Name) == URLParts[0]) {
          if (Sensor.Values.size() > 0) {
            Root = Sensor_t::PrepareValues(Sensor.Values["Primary"]);

            // Дополнительные значения сенсора, кроме Primary. Например - яркость каналов в RGBW Switch
            if (Sensor.Values.size() > 1) {
              for (const auto &Value : Sensor.Values)
                  if (Value.first != "Primary") {
                      Helper = Sensor_t::PrepareValues(Value.second);
                      cJSON_AddItemToObject   (Root, Value.first.c_str(), Helper);
                      cJSON_Delete(Helper);
                  }
            }

            Result->Body = string(cJSON_Print(Root));
            cJSON_Delete(Root);
          }

          break;
        }
    }

    // Запрос строковых значений состояния конкретного сенсора
    // Или - получение JSON дополнительных значений
    if (URLParts.size() == 2)
      for (Sensor_t& Sensor : Sensors)
        if (Tools::ToLower(Sensor.Name) == URLParts[0]) {
          if (URLParts[1] == "value") {
            Result->Body = Sensor.Values["Primary"]["Value"];
            break;
          }

          if (URLParts[1] == "updated") {
            Result->Body = Sensor.Values["Primary"]["Updated"];
            break;
          }

          if (Sensor.Values.size() > 0)
            for (const auto &Value : Sensor.Values) {
              if (Tools::ToLower(Value.first) == URLParts[1])
              {
                Root = Sensor_t::PrepareValues(Value.second);
                Result->Body = string(cJSON_Print(Root));
                cJSON_Delete(Root);
                break;
              }
          break;
      }
    }

    // Запрос строковых значений состояния дополнительного поля конкретного сенсора
    if (URLParts.size() == 3)
      for (Sensor_t& Sensor : Sensors)
        if (Tools::ToLower(Sensor.Name) == URLParts[0])
          for (const auto &Value : Sensor.Values) {
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

cJSON* Sensor_t::PrepareValues(map<string, string> Values) {
  cJSON *JSON = cJSON_CreateObject();

  cJSON_AddStringToObject (JSON, "Value"    , Values["Value"].c_str());
  cJSON_AddStringToObject (JSON, "Updated"  , Values["Updated"].c_str());

  return JSON;
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
  string Value = (GPIO::Read(SWITCH_PIN_NUM) == true) ? "1" : "0";

  map<string,map<string, string>>::iterator ValueIterator = Values.find("Primary");

  if (ValueIterator != Values.end())
    if (Values["Primary"]["Value"] != Value) {
      Values["Primary"]["Value"] = Value;
      Values["Primary"]["Updated"] = Time::GetUnixTime();
    }
}
