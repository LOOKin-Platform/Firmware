#ifndef DEVICE_H
#define DEVICE_H

using namespace std;

#include <string>
#include <vector>
#include <map>
#include <stdio.h>

#include "drivers/NVS/NVS.h"

#include "WebServer.h"
#include "API.h"

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

  private:
    string GenerateID();

    string TypeToString();
    string StatusToString();
    string IDToString();
    string NameToString();
    string PowerModeToString();
    string FirmwareVersionToString();
};

#endif
