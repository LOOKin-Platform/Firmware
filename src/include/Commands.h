#ifndef COMMANDS_H
#define COMMANDS_H

/*
  Команды - классы и функции, связанные с /commands
*/

using namespace std;

#include <string>
#include <vector>
#include <map>
#include <cJSON.h>

#include "Query.h"
#include "Device.h"
#include "WebServer.h"

class Command_t {
  public:
    string              ID;
    string              Name;
    map<string,string>  Events;

    Command_t() {};

    virtual bool                Execute(string Action, map<string,string> Operand = map<string,string>()) { return true; };

    static vector<Command_t*>   GetCommandsForDevice();
    static Command_t*           GetCommandByName(string);
    static WebServerResponse_t* HandleHTTPRequest(QueryType, vector<string>, map<string,string>);
};

class CommandSwitch_t : public Command_t {
  public:
    CommandSwitch_t();
    bool Execute(string Action, map<string,string> Operand) override;
};

#endif
