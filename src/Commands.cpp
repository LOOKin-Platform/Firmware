/*
  Команды - классы и функции, связанные с /commands
*/
#include "include/Globals.h"
#include "include/Commands.h"
#include "include/Device.h"
#include "include/Tools.h"

#include "include/Switch_str.h"

#include <cJSON.h>

#include "drivers/GPIO/GPIO.h"

vector<Command_t> Command_t::GetCommandsForDevice() {
  vector<Command_t> Commands = {};

  switch (Device->Type) {
    case DeviceType::PLUG:
      Commands = { CommandSwitch_t() };
      break;
  }

  return Commands;
}

Command_t Command_t::GetCommandByName(string CommandName) {

  for (Command_t& Command : Commands)
    if (Tools::ToLower(Command.Name) == Tools::ToLower(CommandName))
      return Command;

  return Command_t();
}

WebServerResponse_t* Command_t::HandleHTTPRequest(QueryType Type, vector<string> URLParts, map<string,string> Params) {

    WebServerResponse_t *Result = new WebServerResponse_t();
    cJSON *Root;

    // Вывести список всех сенсоров
    if (URLParts.size() == 0 && Type == QueryType::GET) {
      // Подготовка списка команд для вывода
      vector<const char *> CommandsNames;
      for (Command_t& Command : Commands)
        CommandsNames.push_back(Command.Name.c_str());

      Root = cJSON_CreateStringArray(CommandsNames.data(), CommandsNames.size());
      Result->Body = string(cJSON_Print(Root));
    }

    // Запрос списка действий конкретной команды
    if (URLParts.size() == 1 && Type == QueryType::GET) {
      Command_t Command = Command_t::GetCommandByName(URLParts[0]);

      if (Command.Events.size() > 0)
      {
        vector<const char *> EventsNames;
        for (auto& Event: Command.Events)
          EventsNames.push_back(Event.first.c_str());

        Root = cJSON_CreateStringArray(EventsNames.data(), EventsNames.size());
        Result->Body = string(cJSON_Print(Root));
        cJSON_Delete(Root);
      }
    }

    /*
    GET /switch/on
    POST /switch/on
    POST command=switch&action=on
    */
    if (URLParts.size() == 2 || (URLParts.size() == 0 && Params.size() > 0 && Type == QueryType::POST)) {

      string CommandName = "";
      if (URLParts.size() > 0) CommandName = URLParts[0];
      if (Params.find("command") != Params.end()) {
        CommandName = Params[ "command" ];
        Params.erase ( "command" );
      }

      string Action = "";
      if (URLParts.size() > 1) Action = URLParts[1];
      if (Params.find("action") != Params.end()) {
        CommandName = Params[ "action" ];
        Params.erase ( "action" );
      }

      Command_t Command = Command_t::GetCommandByName(CommandName);
      Command.Execute(Action, Params);
    }

    return Result;
}

/************************************/
/*          Switch Command          */
/************************************/

CommandSwitch_t::CommandSwitch_t() {
    ID          = "00000001";
    Name        = "Switch";

    Events["on"]  = "0001";
    Events["off"] = "0010";
}

void CommandSwitch_t::Execute(string Action, map<string,string> Operand) {

  SWITCH(Action) {
    CASE("on"):
      GPIO::Write(SWITCH_PIN_NUM, true);
      break;
    CASE("off"):
      GPIO::Write(SWITCH_PIN_NUM, false);
      break;
  }

  Sensor_t::UpdateSensors();
}
