/*
*    Commands.h
*    Class to handle API /Commands
*
*/

#ifndef COMMANDS_H
#define COMMANDS_H

using namespace std;

#include <string>
#include <vector>
#include <map>

#include <esp_log.h>

#include "Query.h"
#include "Device.h"
#include "WebServer.h"

class Command_t {
  public:
    uint8_t               ID;
    string                Name;
    map<string,uint8_t>   Events;

    Command_t() {};
    virtual ~Command_t() = default;

    virtual bool                Execute(uint8_t EventCode, uint32_t Operand = 0) { return true; };

    uint8_t                     GetEventCode(string Action);

    static Command_t*           GetCommandByName(string);
    static Command_t*           GetCommandByID(uint8_t);

    static vector<Command_t*>   GetCommandsForDevice();
    static void                 HandleHTTPRequest(WebServerResponse_t* &, QueryType, vector<string>, map<string,string>);
};

class CommandSwitch_t : public Command_t {
  public:
    CommandSwitch_t();
    bool Execute(uint8_t EventCode, uint32_t Operand) override;
};

#endif
