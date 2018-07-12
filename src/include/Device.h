/*
*    Device.h
*    Class to handle API /Device
*
*/

#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <stdio.h>

#include <esp_wifi.h>
#include <esp_log.h>

#include "Settings.h"
#include "WebServer.h"
#include "JSON.h"
#include "Memory.h"
#include "DateTime.h"

#include "OTA.h"
#include "Converter.h"

using namespace std;

#define	NVSDeviceArea				"Device"
#define NVSDeviceType				"Type"
#define NVSDeviceID					"ID"
#define NVSDeviceName				"Name"
#define NVSDevicePowerMode			"PowerMode"
#define	NVSDevicePowerModeVoltage	"PowerModeVoltage"

class DeviceType_t {
	public:
		uint8_t Hex = 0x00;

		DeviceType_t(string);
		DeviceType_t(uint8_t = 0x0);

		bool   IsBattery();
		string ToString();
		string ToHexString();

		static string ToString(uint8_t);
};

enum    DeviceStatus    { RUNNING, UPDATING };
enum    DevicePowerMode { BATTERY, CONST };

class Device_t {
  public:
    DeviceType_t    Type;
    DeviceStatus    Status		= DeviceStatus::RUNNING;

    DevicePowerMode PowerMode	= DevicePowerMode::CONST;
    uint8_t         PowerModeVoltage = 220;

    string          FirmwareVersion = Settings.FirmwareVersion;
    uint8_t         Temperature = 0;

    Device_t();
    void    		Init();
    void    		HandleHTTPRequest(WebServer_t::Response &, QueryType Type, vector<string> URLParts, map<string,string> Params);

    string			GetName();
    void			SetName(string);

    static uint8_t	GetTypeFromNVS();
    static void		SetTypeToNVS(uint8_t);

    static uint32_t	GetIDFromNVS();
    static void		SetIDToNVS(uint32_t);

    string			IDToString();
    string			TypeToString();
    static uint32_t GenerateID();

  private:

    bool 			POSTName(map<string,string>);
    bool 			POSTTime(map<string,string>);
    bool 			POSTTimezone(map<string,string>);
    bool 			POSTFirmwareVersion(map<string,string>, WebServer_t::Response &);

    string 			StatusToString();
    string 			NameToString();
    string 			PowerModeToString();
    string 			FirmwareVersionToString();
    string 			TemperatureToString();
};

#endif
