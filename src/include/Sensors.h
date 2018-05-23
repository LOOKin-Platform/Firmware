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
#include "driver/adc.h"

#include "Globals.h"
#include "WebServer.h"
#include "Automation.h"
#include "JSON.h"
#include "RMT.h"
#include "HardwareIO.h"
#include "DateTime.h"

using namespace std;

struct SensorValueItem {
	uint32_t		Value;
	uint32_t		Updated;
	SensorValueItem(uint32_t Value = 0, uint32_t Updated = 0) : Value(Value), Updated(Updated) {}
};

class Sensor_t {
  public:
    uint8_t           ID;
    string            Name;
    vector<uint8_t>   EventCodes;

    map<string, SensorValueItem> Values;
    virtual ~Sensor_t() = default;

    virtual void			Update() {};
    virtual uint32_t		ReceiveValue(string = "") { return 0; };
    virtual bool			CheckOperand(uint8_t, uint8_t) { return false; };
    virtual string			FormatValue(string Key = "Primary") { return Converter::ToString(GetValue(Key).Value); };

    static void				UpdateSensors();
    static vector<Sensor_t*>GetSensorsForDevice();
    static Sensor_t*		GetSensorByName(string);
    static Sensor_t*		GetSensorByID(uint8_t);
    static uint8_t			GetDeviceTypeHex();

    bool					SetValue(uint32_t Value, string Key = "Primary");
    SensorValueItem			GetValue(string Key = "Primary");

    static void HandleHTTPRequest(WebServer_t::Response &, QueryType, vector<string>, map<string,string>);
};

extern Settings_t 	Settings;
extern Automation_t Automation;
extern WebServer_t  WebServer;

#include "../sensors/SensorSwitch.cpp"
#include "../sensors/SensorRGBW.cpp"
#include "../sensors/SensorIR.cpp"
#include "../sensors/SensorMotion.cpp"

#endif
