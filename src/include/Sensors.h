/*
*    Sensors.h
*    Class to handle API /Sensors
*
*/

#ifndef SENSORS_H
#define SENSORS_H

#include <string>
#include <vector>
#include <map>
#include <bitset>

#include <esp_log.h>

#include "JSON.h"
#include "IR.h"
#include "HardwareIO.h"
#include "DateTime.h"
#include "WebServer.h"

using namespace std;

struct SensorValueItem {
  double    Value;
  uint32_t  Updated;
  SensorValueItem(double Value = 0, uint32_t Updated = 0) : Value(Value), Updated(Updated) {}
};

class Sensor_t {
  public:
    uint8_t           ID;
    string            Name;
    vector<uint8_t>   EventCodes;

    map<string, SensorValueItem> Values;

    virtual void                Update() {};
    virtual double              ReceiveValue(string = "") { return 0; };
    virtual bool                CheckOperand(uint8_t, uint8_t) { return false; };
    virtual string              FormatValue(string Key = "Primary") { return Converter::ToString(GetValue(Key).Value); };

    static void                 UpdateSensors();
    static vector<Sensor_t*>    GetSensorsForDevice();
    static Sensor_t*            GetSensorByName(string);
    static Sensor_t*            GetSensorByID(uint8_t);

    bool                        SetValue(double Value, string Key = "Primary");
    SensorValueItem             GetValue(string Key = "Primary");

    static void HandleHTTPRequest(WebServer_t::Response &, QueryType, vector<string>, map<string,string>);
};

class SensorSwitch_t : public Sensor_t {
  public:
    SensorSwitch_t();
    void    Update()                            override;
    double  ReceiveValue(string = "Primary")    override;
    bool    CheckOperand(uint8_t, uint8_t)      override;

};

class SensorColor_t : public Sensor_t {
  public:
    SensorColor_t();
    void    Update()                            override;
    double  ReceiveValue(string = "Primary")    override;
    string  FormatValue(string Key = "Primary") override;
    bool    CheckOperand(uint8_t, uint8_t)      override;

    static uint8_t ToBrightness(uint8_t Red, uint8_t Green, uint8_t Blue);
    static uint8_t ToBrightness(uint32_t Color);
};

class SensorIR_t : public Sensor_t {
  public:
	SensorIR_t();
    void    Update()                            override;
    double  ReceiveValue(string = "Primary")    override;
    string  FormatValue(string Key = "Primary") override;
    bool    CheckOperand(uint8_t, uint8_t)      override;

    static void MessageStart();
    static void MessageBody(int16_t Bit);
    static void MessageEnd();
  private:
    static vector<int16_t> CurrentMessage;

    static bool CheckNEC();
    static void ParseNEC(uint16_t &Adress, uint8_t &Command);
};

#endif
