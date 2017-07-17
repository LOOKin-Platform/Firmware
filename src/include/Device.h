#ifndef DEVICE_H
#define DEVICE_H

using namespace std;

#include <string>
#include <vector>
#include <map>
#include <stdio.h>

#include <esp_wifi.h>
#include <esp_log.h>

#include "drivers/NVS/NVS.h"

#include "WebServer.h"
#include "API.h"

#define  NVSDeviceType              "Type"
#define  NVSDeviceID                "ID"
#define  NVSDeviceName              "Name"
#define  NVSDevicePowerMode         "PowerMode"
#define  NVSDevicePowerModeVoltage  "PowerModeVoltage"
#define  NVSDeviceFirmwareVersion   "FirmwareVersion"


enum    DeviceType      { PLUG };
enum    DeviceStatus    { RUNNING, UPDATING };
enum    DevicePowerMode { BATTERY, CONST };

class Device_t : public API {
  public:
    DeviceType      Type;
    DeviceStatus    Status;
    string          ID;
    string          Name;

    DevicePowerMode PowerMode;
    uint8_t         PowerModeVoltage;

    string          FirmwareVersion;

    Device_t();
    void    Init();
    WebServerResponse_t*  HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params);

    string TypeToString();
  private:
    string GenerateID();

    string StatusToString();
    string IDToString();
    string NameToString();
    string PowerModeToString();
    string FirmwareVersionToString();
};

#endif
