/*
*    Sensors.cpp
*    Class to handle API /Sensors
*
*/
#include "Globals.h"
#include "Sensors.h"

static char tag[] = "Sensors";

vector<Sensor_t*> Sensor_t::GetSensorsForDevice() {
  vector<Sensor_t*> Sensors = {};

  switch (Device->Type->Hex) {
    case DEVICE_TYPE_PLUG_HEX:
      Sensors = { new SensorSwitch_t(), new SensorColor_t() };
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

  return nullptr;
}

void Sensor_t::UpdateSensors() {
  for (auto& Sensor : Sensors)
    Sensor->Update();
}

void Sensor_t::HandleHTTPRequest(WebServerResponse_t* &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {

    Sensor_t::UpdateSensors();

    // Echo list of all sensors
    if (URLParts.size() == 0) {

      vector<string> Vector = vector<string>();
      for (auto& Sensor : Sensors)
        Vector.push_back(Sensor->Name);

      Result->Body = JSON::CreateStringFromVector(Vector);
    }

    // Запрос значений конкретного сенсора
    if (URLParts.size() == 1) {

      Sensor_t* Sensor = Sensor_t::GetSensorByName(URLParts[0]);

      if (Sensor->Values.size() > 0) {

        JSON JSONObject;

        JSONObject.SetItems({
          { "Value"               , Sensor->FormatValue()  },
          { "Updated"             , Converter::ToString(Sensor->Values["Primary"].Updated)}
        });

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

        Result->Body = JSONObject.ToString();
      }
    }

    // Запрос строковых значений состояния конкретного сенсора
    // Или - получение JSON дополнительных значений
    if (URLParts.size() == 2)
      for (Sensor_t* Sensor : Sensors)
        if (Converter::ToLower(Sensor->Name) == URLParts[0]) {
          if (URLParts[1] == "value") {
            Result->Body = Sensor->FormatValue();
            break;
          }

          if (URLParts[1] == "updated") {
            Result->Body = Converter::ToString(Sensor->Values["Primary"].Updated);
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

                Result->Body = JSONObject.ToString();
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
              if (URLParts[2] == "value")   Result->Body = Sensor->FormatValue(Value.first);
              if (URLParts[2] == "updated") Result->Body = Converter::ToString(Value.second.Updated);

              break;
            }
          }
}

// возвращаемое значение - было ли изменено значение в памяти
bool Sensor_t::SetValue(double Value, string Key) {
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

/************************************/
/*          Switch Sensor           */
/************************************/

SensorSwitch_t::SensorSwitch_t() {
    ID          = 0x81;
    Name        = "Switch";
    EventCodes  = { 0x00, 0x01, 0x02 };
}

void SensorSwitch_t::Update() {
  if (SetValue(ReceiveValue())) {
    WebServer->UDPSendBroadcastUpdated(ID, Converter::ToString(GetValue().Value));
    Automation->SensorChanged(ID);
  }
}

double SensorSwitch_t::ReceiveValue(string Key) {
  switch (Device->Type->Hex) {
    case DEVICE_TYPE_PLUG_HEX:
      return (GPIO::Read(SWITCH_PLUG_PIN_NUM) == true) ? 1 : 0;
      break;
    default: return 0;
  }
}

bool SensorSwitch_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
  SensorValueItem ValueItem = GetValue();

  if (SceneEventCode == 0x01 && ValueItem.Value == 1) return true;
  if (SceneEventCode == 0x02 && ValueItem.Value == 0) return true;

  return false;
}


/************************************/
/*           Color Sensor           */
/************************************/

SensorColor_t::SensorColor_t() {
    ID          = 0x84;
    Name        = "RGBW";
    EventCodes  = { 0x00, 0x02, 0x03, 0x04 };
}

void SensorColor_t::Update() {

  SetValue(ReceiveValue("Red")  , "Red");
  SetValue(ReceiveValue("Green"), "Green");
  SetValue(ReceiveValue("Blue") , "Blue");
  SetValue(ReceiveValue("White"), "White");

  double Color = (double)floor(GetValue("Red").Value * 255 * 255 + GetValue("Green").Value * 255 + GetValue("Blue").Value);

  if (SetValue(Color)) {
    WebServer->UDPSendBroadcastUpdated(ID, Converter::ToHexString(Color,6));
    Automation->SensorChanged(ID);
  }
}

double SensorColor_t::ReceiveValue(string Key) {
  switch (Device->Type->Hex) {
    case DEVICE_TYPE_PLUG_HEX:
      if (Key == "Red")     return GPIO::PWMValue(COLOR_PLUG_RED_PWMCHANNEL);
      if (Key == "Green")   return GPIO::PWMValue(COLOR_PLUG_GREEN_PWMCHANNEL);
      if (Key == "Blue")    return GPIO::PWMValue(COLOR_PLUG_BLUE_PWMCHANNEL);
      if (Key == "White")   return GPIO::PWMValue(COLOR_PLUG_WHITE_PWMCHANNEL);
      if (Key == "Primary") return (double)floor(GetValue("Red").Value * 255 * 255 + GetValue("Green").Value * 255 + GetValue("Blue").Value);
      break;
  }

  return 0;
}

string SensorColor_t::FormatValue(string Key) {
  if (Key == "Primary")
    return Converter::ToHexString(Values[Key].Value, 6);
  else
    return Converter::ToHexString(Values[Key].Value, 2);
}

bool SensorColor_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
  double Red    = GetValue("Red").Value;
  double Green  = GetValue("Green").Value;
  double Blue   = GetValue("Blue").Value;

  // Установленная яркость равна значению в сценарии
  if (SceneEventCode == 0x02 && SceneEventOperand == ToBrightness(Red, Green, Blue))
    return true;

  // Установленная яркость меньше значения в сценарии
  if (SceneEventCode == 0x03 && SceneEventOperand > ToBrightness(Red, Green, Blue))
    return true;

  // Установленная яркость меньше значения в сценарии
  if (SceneEventCode == 0x04 && SceneEventOperand < ToBrightness(Red, Green, Blue))
    return true;

  return false;
}

uint8_t SensorColor_t::ToBrightness(uint32_t Color) {
  uint8_t oBlue    = Color&0x000000FF;
  uint8_t oGreen   = (Color&0x0000FF00)>>8;
  uint8_t oRed     = (Color&0x00FF0000)>>16;

  return ToBrightness(oRed, oGreen, oBlue);
}

uint8_t SensorColor_t::ToBrightness(uint8_t Red, uint8_t Green, uint8_t Blue) {
  return max(max(Red,Green), Blue);
}
