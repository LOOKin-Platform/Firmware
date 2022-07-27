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

#include "Query.h"

#include "OTA.h"
#include "Converter.h"

using namespace std;

#define	NVSDeviceArea				"Device"
#define NVSDeviceType				"Type"
#define NVSDeviceName				"Name"
#define NVSDevicePowerMode			"PowerMode"
#define	NVSDevicePowerModeVoltage	"PowerModeVoltage"
#define	NVSDeviceSensorMode			"SensorMode"
#define	NVSDeviceEco				"Eco"
#define	NVSDeviceAutoUpdate			"IsAutoUpdate"

#define NVSDeviceID					"ID"
#define NVSDeviceType				"Type"
#define NVSDeviceModel				"Model"
#define NVSDeviceRevision			"Revision"

class DeviceType_t {
	public:
		uint8_t Hex = 0x00;

		DeviceType_t(string);
		DeviceType_t(uint8_t = 0x0);

		bool   			IsBattery();
		string 			ToString();
		string 			ToHexString();

		static string 	ToString(uint8_t);
};

enum    DeviceStatus    { RUNNING, UPDATING };
enum    DevicePowerMode { BATTERY, CONST };

class Device_t {
	public:
		DeviceType_t    	Type;
		DeviceStatus    	Status		= DeviceStatus::RUNNING;

		DevicePowerMode 	PowerMode	= DevicePowerMode::CONST;
		uint8_t         	PowerModeVoltage = 220;

		uint8_t         	Temperature = 0;
		uint16_t			CurrentVoltage = 0;

		bool				SensorMode = false;

		Device_t();
		void    			Init();
		void    			HandleHTTPRequest(WebServer_t::Response &, Query_t &);
		JSON				RootInfo();

		string				GetName();
		void				SetName(string);

		static bool 		GetEcoFromNVS();
		static void 		SetEcoToNVS(bool);

		static bool			GetSensorModeFromNVS();
		static void			SetSensorModeToNVS(bool);

		static bool 		GetAutoUpdateFromNVS();
		static void 		SetAutoUpdateToNVS(bool);

		string				IDToString();
		string				TypeToString();
		string				ModelToString();
		string				HomeKitToString();

		static uint32_t 	GenerateID();

		static void			OTAStartedCallback();
		static void			OTAFailedCallback();
		static	httpd_req_t	*CachedRequest;

		void 				OTAStart(string FirmwareURL, WebServer_t::QueryTransportType TransportType = WebServer_t::QueryTransportType::Undefined);

		static inline bool 	IsAutoUpdate = true;

	private:
		bool 				POSTName(map<string,string>);
		bool 				POSTTime(map<string,string>);
		bool 				POSTTimezone(map<string,string>);
		bool 				POSTFirmwareVersion(map<string,string>, WebServer_t::Response &, httpd_req_t *Request, WebServer_t::QueryTransportType);
		bool 				POSTSensorMode(map<string,string>, WebServer_t::Response &);
		bool 				POSTEco(map<string,string> Params);
		bool				POSTIsAutoUpdate(Query_t& Query);

		string 				StatusToString();
		string 				NameToString();
		string 				PowerModeToString();
		string 				FirmwareVersionToString();
		string 				TemperatureToString();
		string 				CurrentVoltageToString();
		string				SensorModeToString();
		string				MRDCToString();
		string				EcoToString();

		static string 		FirmwareURLForOTA;
		static TaskHandle_t	OTATaskHandler;
		static void 		ExecuteOTATask(void*);
};

#endif
