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

    virtual void                Execute(string Action, map<string,string> Operand = map<string,string>()) {};

    static vector<Command_t>    GetCommandsForDevice();
    static Command_t            GetCommandByName(string CommandName);
    static WebServerResponse_t* HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params);
};

class CommandSwitch_t : public Command_t {
  public:
    CommandSwitch_t();
    void Execute(string Action, map<string,string> Operand) override;
};

#endif
