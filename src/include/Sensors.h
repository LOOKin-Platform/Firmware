#ifndef SENSORS_H
#define SENSORS_H

/*
  Сенсоры - классы и функции, связанные с /sensors
*/

using namespace std;

#include <string>
#include <vector>
#include <map>
#include <cJSON.h>

#include "include/Device.h"

class Sensor_t {
  public:
    string          ID;
    string          Name;
    vector<string>  EventCodes;

    map<string, map<string, string> > Values;

    virtual void Update() {};

    static void UpdateSensors();
    static vector<Sensor_t> GetSensorsForDevice();

    static string HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params);
    static cJSON* PrepareValues(map<string, string> ValuesMap);
};

class SensorSwitch_t : public Sensor_t {
  public:

    SensorSwitch_t();
    void Update() override;
};

#endif
