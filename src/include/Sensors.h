#ifndef SENSORS_H
#define SENSORS_H

/*
  Сенсоры - классы и функции, связанные с /sensors
*/

#include "JSON.h"

#include <string>
#include <vector>
#include <map>

#include <esp_log.h>

#include "Device.h"
#include "WebServer.h"

using namespace std;
using namespace rapidjson;

class Sensor_t {
  public:
    string          ID;
    string          Name;
    vector<string>  EventCodes;

    map<string, map<string, string> > Values;

    virtual void                Update() {};

    static void                 UpdateSensors();
    static vector<Sensor_t*>    GetSensorsForDevice();
    static Sensor_t*            GetSensorByName(string);

    static WebServerResponse_t* HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params);
    static void                 PrepareValues(map<string, string>, Writer<StringBuffer> &Writer);
};

class SensorSwitch_t : public Sensor_t {
  public:

    SensorSwitch_t();
    void Update() override;
};

#endif
