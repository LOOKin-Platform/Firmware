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
          {"Value"               , Sensor->Values["Primary"]["Value"]},
          {"Updated"             , Sensor->Values["Primary"]["Updated"]}
        });

        // Дополнительные значения сенсора, кроме Primary. Например - яркость каналов в RGBW Switch
        if (Sensor->Values.size() > 1) {
          for (const auto &Value : Sensor->Values)
            if (Value.first != "Primary") {
              map<string,string> SensorValue = Value.second;

              JSONObject.SetObject(Value.first, {
                { "Value"   , SensorValue["Value"]},
                { "Updated" , SensorValue["Updated"]}
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
                JSON JSONObject;

                map<string,string> SensorValue = Value.second;
                JSONObject.SetObject(Value.first, {
                  { "Value"   , SensorValue["Value"]  },
                  { "Updated" , SensorValue["Updated"]}
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

void SensorSwitch_t::Update(uint32_t Operand) {
  string newValue = (GPIO::Read(SWITCH_PLUG_PIN_NUM) == true) ? "1" : "0";
  string oldValue = "";

  if (Values.count("Primary"))
    oldValue = Values["Primary"]["Value"];

  if (oldValue != newValue || Values.count("Primary") == 0) {
    Values["Primary"]["Value"]    = newValue;
    Values["Primary"]["Updated"]  = Time::UnixtimeString();

    WebServer->UDPSendBroadcastUpdated(ID,newValue);
    Automation->SensorChanged(ID, (newValue == "1") ? 0x01 : 0x02 );
  }
}

bool SensorSwitch_t::CheckOperand(uint8_t SceneEventCode, uint8_t EventCode, uint8_t SceneEventOperand, uint8_t EventOperand) {
  return (SceneEventCode == EventCode);
}


/************************************/
/*           Color Sensor           */
/************************************/

SensorColor_t::SensorColor_t() {
    ID          = 0x84;
    Name        = "RGBW";
    EventCodes  = { 0x00, 0x02, 0x03, 0x04 };
}

void SensorColor_t::Update(uint32_t Operand) {

  Red = 0, Green = 0, Blue = 0, White = 0;

  if (Operand > 0) {
    Blue    = Operand&0x000000FF;
    Green   = (Operand&0x0000FF00)>>8;
    Red     = (Operand&0x00FF0000)>>16;
  }
  else
    switch (Device->Type->Hex) {
      case DEVICE_TYPE_PLUG_HEX:
        Red   = GPIO::PWMValue(COLOR_PLUG_RED_PWMCHANNEL);
        Green = GPIO::PWMValue(COLOR_PLUG_GREEN_PWMCHANNEL);
        Blue  = GPIO::PWMValue(COLOR_PLUG_BLUE_PWMCHANNEL);
        White = GPIO::PWMValue(COLOR_PLUG_WHITE_PWMCHANNEL);
        break;
    }

  string newValue = Converter::ToHexString(Red, 2) + Converter::ToHexString(Green, 2) + Converter::ToHexString(Blue, 2);
  string oldValue = "";

  if (Values.count("Primary"))
    oldValue = Values["Primary"]["Value"];

  if (oldValue != newValue || Values.count("Primary") == 0) {
    Values["Primary"]["Value"]    = newValue;
    Values["Primary"]["Updated"]  = Time::UnixtimeString();

    WebServer->UDPSendBroadcastUpdated(ID, newValue); // отправляем широковещательно новый цвет
    Automation->SensorChanged(ID, 0x02, ToBrightness(Red, Green, Blue)); // в условия автоматизации отправляем яркость
  }

  string RedValue = Converter::ToHexString(Red, 2);
  if (Values.count("Red") == 0 || Values["Red"]["Value"] != RedValue) {
    Values["Red"]["Value"] = RedValue;
    Values["Red"]["Updated"]  = Time::UnixtimeString();
  }

  string GreenValue = Converter::ToHexString(Green,2);
  if (Values.count("Green") == 0 || Values["Green"]["Value"] != GreenValue) {
    Values["Green"]["Value"] = RedValue;
    Values["Green"]["Updated"]  = Time::UnixtimeString();
  }

  string BlueValue = Converter::ToHexString(Blue,2);
  if (Values.count("Blue") == 0 || Values["Blue"]["Value"] != BlueValue) {
    Values["Blue"]["Value"]     = BlueValue;
    Values["Blue"]["Updated"]   = Time::UnixtimeString();
  }

  string WhiteValue = Converter::ToHexString(White,2);
  if (Values.count("Red") == 0 || Values["White"]["Value"] != WhiteValue) {
    Values["White"]["Value"] = RedValue;
    Values["White"]["Updated"]  = Time::UnixtimeString();
  }
}

bool SensorColor_t::CheckOperand(uint8_t SceneEventCode, uint8_t EventCode, uint8_t SceneEventOperand, uint8_t EventOperand) {
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
