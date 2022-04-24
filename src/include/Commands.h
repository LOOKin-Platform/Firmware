/*
*    Commands.h
*    Class to handle API /Commands
*
*/

#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>
#include <map>

#include <esp_log.h>

#include "Settings.h"

#include "Globals.h"
#include "Device.h"
#include "WebServer.h"
#include "JSON.h"
#include "HardwareIO.h"
#include "Converter.h"
#include "Storage.h"

#include "Sensors.h"

#include "Log.h"

using namespace std;

class Command_t {
	public:
		uint8_t               		ID;
		string                		Name;
		map<string,uint8_t>   		Events;

		bool						InOperation = false;

		virtual ~Command_t() = default;

		virtual bool				Execute(uint8_t EventCode, const char* StringOperand) { return true; };
		virtual void				Overheated() {};

		uint8_t						GetEventCode(string Action);

		static Command_t*			GetCommandByName(string);
		static Command_t*			GetCommandByID(uint8_t);
		static uint8_t				GetDeviceTypeHex();

		virtual void				InitSettings()				{ };
		virtual string				GetSettings() 				{ return ""; };
		virtual void				SetSettings(WebServer_t::Response &Result, Query_t &Query) { Result.SetInvalid(); }

		static vector<Command_t*>	GetCommandsForDevice();
		static void					HandleHTTPRequest(WebServer_t::Response &, Query_t &Query);

		static void					SendLocalMQTT(string Payload, string Topic);
};

extern Storage_t Storage;

#include "../commands/CommandSwitch.cpp"
#include "../commands/CommandMultiSwitch.cpp"
//#include "../commands/CommandRGBW.cpp"
#include "../commands/CommandIR.cpp"
#include "../commands/CommandBLE.cpp"

#endif
