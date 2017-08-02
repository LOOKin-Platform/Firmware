#ifndef DEVICE_H
#define DEVICE_H

/*
  Классы для работы с API /device
*/

using namespace std;

#include "Settings.h"

#include <string>
#include <vector>
#include <map>
#include <stdio.h>

#include <esp_wifi.h>
#include <esp_log.h>

#include "NVS/NVS.h"

#include "WebServer.h"
#include "API.h"
#include "Converter.h"

#define  NVSDeviceType              "Type"
#define  NVSDeviceID                "ID"
#define  NVSDeviceName              "Name"
#define  NVSDevicePowerMode         "PowerMode"
#define  NVSDevicePowerModeVoltage  "PowerModeVoltage"

class DeviceType_t {
  public:
    uint8_t Hex = 0x00;    // : 3   (3 bits)

    DeviceType_t(string);
    DeviceType_t(uint8_t);

    bool   IsBattery();
    string ToString();
    string ToHexString();

    static string ToString(uint8_t);
    static map<uint8_t, string> TypeMap;
};

enum    DeviceStatus    { RUNNING, UPDATING };
enum    DevicePowerMode { BATTERY, CONST };

class Device_t : public API {
  public:
    DeviceType_t    *Type;
    DeviceStatus    Status;
    uint32_t        ID;
    string          Name;

    DevicePowerMode PowerMode;
    uint8_t         PowerModeVoltage;

    string          FirmwareVersion;

    Device_t();
    void    Init();
    void    HandleHTTPRequest(WebServerResponse_t* &, QueryType Type, vector<string> URLParts, map<string,string> Params);

    string TypeToString();
    string IDToString();
  private:
    uint32_t GenerateID();

    bool POSTName(map<string,string>);
    bool POSTTime(map<string,string>);
    bool POSTTimezone(map<string,string>);
    bool POSTFirmwareVersion(map<string,string>, WebServerResponse_t &);

    string StatusToString();
    string NameToString();
    string PowerModeToString();
    string FirmwareVersionToString();
};

#endif
