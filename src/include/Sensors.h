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

#include "Device.h"
#include "WebServer.h"

using namespace std;

class Sensor_t {
  public:
    uint8_t           ID;
    string            Name;
    vector<uint8_t>   EventCodes;

    map<string, map<string, string>> Values;

    virtual void                Update() {};

    static void                 UpdateSensors();
    static vector<Sensor_t*>    GetSensorsForDevice();
    static Sensor_t*            GetSensorByName(string);

    static void HandleHTTPRequest(WebServerResponse_t* &, QueryType, vector<string>, map<string,string>);
};

class SensorSwitch_t : public Sensor_t {
  public:
    SensorSwitch_t();
    void Update() override;
};

#endif
