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

#include "Settings.h"

#include "Globals.h"
#include "Wireless.h"
#include "Automation.h"
#include "JSON.h"
#include "RMT.h"
#include "HardwareIO.h"
#include "DateTime.h"
#include "Commands.h"
#include "Storage.h"
#include "Log.h"

using namespace std;

struct SensorValueItem {
	uint32_t		Value;
	uint32_t		Updated;
	SensorValueItem(uint32_t Value = 0, uint32_t Updated = 0) : Value(Value), Updated(Updated) {}
};

class Sensor_t {
	public:
		uint8_t           	ID = 0x0;
		string            	Name;
		vector<uint8_t>   	EventCodes;

		map<string, SensorValueItem> Values;
		Sensor_t();
		virtual ~Sensor_t() = default;

		virtual void				Update() {};
		virtual uint32_t			ReceiveValue(string = "") 			{ return 0; };
		virtual bool				CheckOperand(uint8_t, uint8_t) 		{ return false; };
		virtual string				FormatValue(string Key = "Primary") { return Converter::ToString(GetValue(Key).Value); };

		bool						SetValue(uint32_t Value, string Key = "Primary", uint32_t UpdatedTime = 0);
		SensorValueItem				GetValue(string Key = "Primary");

		virtual string				StorageEncode(map<string,string>) 	{ return ""; };
		virtual map<string,string>	StorageDecode(string) 				{ return map<string,string> ();};

		bool						GetIsInited() 						{ return IsInited; 	}
		void						SetIsInited(bool V)					{ IsInited = V; 	};

		virtual void				Pool()								{ }

		string						EchoSummaryJSON();
		virtual string				SummaryJSON()						{ return ""; };

		static void					UpdateSensors();
		static vector<Sensor_t*>	GetSensorsForDevice();
		static Sensor_t*			GetSensorByName(string);
		static Sensor_t*			GetSensorByID(uint8_t);
		static uint8_t				GetDeviceTypeHex();

		static void HandleHTTPRequest(WebServer_t::Response &, QueryType, vector<string>, map<string,string>);

	private:
		bool 						IsInited	= false;
};

extern Settings_t 	Settings;
extern Automation_t Automation;
extern Wireless_t	Wireless;
extern Storage_t	Storage;

#include "../sensors/SensorSwitch.cpp"
#include "../sensors/SensorMultiSwitch.cpp"
#include "../sensors/SensorRGBW.cpp"
#include "../sensors/SensorIR.cpp"
#include "../sensors/SensorMotion.cpp"
#include "../sensors/SensorTemperature.cpp"

#endif
