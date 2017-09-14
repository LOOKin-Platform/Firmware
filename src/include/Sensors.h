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

#include <esp_log.h>

#include "Convert.h"

#include "HardwareIO.h"
#include "DateTime.h"
#include "JSON.h"

#include "WebServer.h"

using namespace std;

class Sensor_t {
  public:
    uint8_t           ID;
    string            Name;
    vector<uint8_t>   EventCodes;

    map<string, map<string, string>> Values;

    virtual void                Update(uint32_t Operand = 0) {};
    virtual bool                CheckOperand(uint8_t, uint8_t, uint8_t, uint8_t) { return false; };

    static void                 UpdateSensors();
    static vector<Sensor_t*>    GetSensorsForDevice();
    static Sensor_t*            GetSensorByName(string);
    static Sensor_t*            GetSensorByID(uint8_t);

    static void HandleHTTPRequest(WebServerResponse_t* &, QueryType, vector<string>, map<string,string>);
};

class SensorSwitch_t : public Sensor_t {
  public:
    SensorSwitch_t();
    void Update(uint32_t Operand = 0) override;
    bool CheckOperand(uint8_t, uint8_t, uint8_t, uint8_t) override;
};


class SensorColor_t : public Sensor_t {
  public:
    SensorColor_t();
    void Update(uint32_t Operand = 0) override;
    bool CheckOperand(uint8_t, uint8_t, uint8_t, uint8_t) override;

    static uint8_t ToBrightness(uint8_t Red, uint8_t Green, uint8_t Blue);
    static uint8_t ToBrightness(uint32_t Color);

  private:
    uint8_t Red, Green, Blue, White;
};

#endif
