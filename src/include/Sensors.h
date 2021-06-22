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

#include "Automation.h"
#include "Globals.h"
#include "Wireless.h"
#include "JSON.h"
#include "RMT.h"
#include "HardwareIO.h"
#include "DateTime.h"
#include "Commands.h"
#include "LocalMQTT.h"
#include "Storage.h"
#include "Log.h"

using namespace std;

class Sensor_t {
	public:
		uint8_t           			ID = 0x0;
		string            			Name;
		vector<uint8_t>   			EventCodes;

		map<string, uint32_t> 		Values = map<string, uint32_t>();
		uint32_t					Updated = 0;

		Sensor_t();
		virtual ~Sensor_t() = default;

		virtual void				Update() {};
		virtual uint32_t			ReceiveValue(string = "") 			{ return 0; };
		virtual bool				CheckOperand(uint8_t, uint8_t) 		{ return false; };
		virtual string				FormatValue(string Key = "Primary") { return Converter::ToString(GetValue(Key)); };

		virtual bool				HasPrimaryValue() 					{ return true; }
		virtual bool				ShouldUpdateInMainLoop()			{ return true; }

		bool						SetValue(uint32_t Value, string Key, uint32_t UpdatedTime);
		bool						SetValue(uint32_t Value, string Key);

		uint32_t					GetValue(string Key = "Primary");

		virtual string				StorageEncode(map<string,string>) 	{ return ""; };
		virtual map<string,string>	StorageDecode(string) 				{ return map<string,string> ();};

		bool						GetIsInited() 						{ return IsInited; 	}
		void						SetIsInited(bool V)					{ IsInited = V; 	};

		virtual void				Pool()								{ }

		string						RootSensorJSON();
		string						EchoSummaryJSON();
		virtual string				SummaryJSON()						{ return ""; };

		static void					UpdateSensors();
		static vector<Sensor_t*>	GetSensorsForDevice();
		static Sensor_t*			GetSensorByName(string);
		static Sensor_t*			GetSensorByID(uint8_t);
		static uint8_t				GetDeviceTypeHex();

		static void					LocalMQTTSend(string Payload, string Topic);

		static void HandleHTTPRequest(WebServer_t::Response &, Query_t &);

	private:
		bool 						IsInited	= false;

	protected:
		bool						IsHomeKitEnabled();
		bool 						IsHomeKitExperimental();

};

extern Settings_t 	Settings;
extern Automation_t Automation;
extern Wireless_t	Wireless;
extern Storage_t	Storage;

//#include "../sensors/SensorSwitch.cpp"
#include "../sensors/SensorMultiSwitch.cpp"
//#include "../sensors/SensorRGBW.cpp"
#include "../sensors/SensorIR.cpp"
//#include "../sensors/SensorMotion.cpp"
#include "../sensors/SensorTemperature.cpp"
#include "../sensors/SensorMeteo.cpp"
#include "../sensors/SensorTouch.cpp"

#endif
