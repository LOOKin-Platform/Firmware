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
#include "Sensors.h"

using namespace std;

class Command_t {
  public:
    uint8_t               		ID;
    string                		Name;
    map<string,uint8_t>   		Events;

    virtual ~Command_t() = default;

    virtual bool				Execute(uint8_t EventCode, string StringOperand = "0") { return true; };
    virtual void				Overheated() {};

    uint8_t						GetEventCode(string Action);

    static Command_t*			GetCommandByName(string);
    static Command_t*			GetCommandByID(uint8_t);
    static uint8_t				GetDeviceTypeHex();

    static vector<Command_t*>	GetCommandsForDevice();
    static void					HandleHTTPRequest(WebServer_t::Response &, QueryType, vector<string>, map<string,string>);
};

#include "../commands/CommandSwitch.cpp"
#include "../commands/CommandRGBW.cpp"
#include "../commands/CommandIR.cpp"

#endif
