/*
  Команды - классы и функции, связанные с /commands
*/

#include <RapidJSON.h>

#include "Globals.h"
#include "Commands.h"
#include "Device.h"
#include "Converter.h"
#include "JSON.h"

#include "GPIO/GPIO.h"

static char tag[] = "Commands";

uint8_t Command_t::GetEventCode(string Action) {
  if (Events.count(Action) > 0)
    return Events[Action];

    for(auto const &Event : Events)
      if (Event.second == Converter::UintFromHexString<uint8_t>(Action))
        return Event.second;

  return +0;
}

Command_t* Command_t::GetCommandByName(string CommandName) {
  for (Command_t* Command : Commands)
    if (Converter::ToLower(Command->Name) == Converter::ToLower(CommandName))
      return Command;

  return nullptr;
}

Command_t* Command_t::GetCommandByID(uint8_t CommandID) {
  for (Command_t* Command : Commands)
    if (Command->ID == CommandID)
      return Command;

  return nullptr;
}

vector<Command_t*> Command_t::GetCommandsForDevice() {
  vector<Command_t*> Commands = {};

  switch (Device->Type->Hex) {
    case DEVICE_TYPE_PLUG_HEX:
      Commands = { new CommandSwitch_t() };
      break;
  }

  return Commands;
}

void Command_t::HandleHTTPRequest(WebServerResponse_t* &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
    // Вывести список всех комманд
    if (URLParts.size() == 0 && Type == QueryType::GET) {
      JSON_t *JSON = new JSON_t();

      for (Command_t* Command : Commands)
        JSON->AddToVector(Command->Name);

      Result->Body = JSON->ToString(true);
      delete JSON;
    }

    // Запрос списка действий конкретной команды
    if (URLParts.size() == 1 && Type == QueryType::GET) {
      Command_t* Command = Command_t::GetCommandByName(URLParts[0]);

      if (Command!=nullptr)
        if (Command->Events.size() > 0)
        {
          JSON_t *JSON = new JSON_t();

          for (auto& Event: Command->Events)
            JSON->AddToVector(Event.first);

          Result->Body = JSON->ToString(true);

          delete JSON;
        }
    }

    /*
    GET /switch/on
    POST /switch/on
    POST command=switch&action=on
    */
    if (URLParts.size() == 2 || (URLParts.size() == 0 && Params.size() > 0 && Type != QueryType::DELETE)) {

      string CommandName = "";
      if (URLParts.size() > 0) CommandName = URLParts[0];

      if (Params.find("command") != Params.end()) {
        CommandName = Params[ "command" ];
        Params.erase ( "command" );
      }

      string Action = "";
      if (URLParts.size() > 1) Action = URLParts[1];
      if (Params.find("action") != Params.end()) {
        Action = Params[ "action" ];
        Params.erase ( "action" );
      }

      string Operand = "0";
      if (Params.find("operand") != Params.end()) {
        Operand = Params[ "operand" ];
        Params.erase ( "operand" );
      }

      Command_t* Command = Command_t::GetCommandByName(CommandName);
      if (Command == nullptr)
        Command = Command_t::GetCommandByID(Converter::UintFromHexString<uint8_t>(CommandName));

      if (Command != nullptr) {
          if (Command->Execute(Command->GetEventCode(Action), Converter::UintFromHexString<uint32_t>(Operand)))
            Result->SetSuccess();
          else
            Result->SetFail();
        }
    }
}

/************************************/
/*          Switch Command          */
/************************************/

CommandSwitch_t::CommandSwitch_t() {
    ID          = 0x01;
    Name        = "Switch";

    Events["on"]  = 0x01;
    Events["off"] = 0x02;

    GPIO::Setup(SWITCH_PLUG_PIN_NUM);
}

bool CommandSwitch_t::Execute(uint8_t EventCode, uint32_t Operand) {
  bool Executed = false;

  if (EventCode == 0x00)
    return false;

  if (EventCode == 0x01) {
    GPIO::Write(SWITCH_PLUG_PIN_NUM, true);
    Executed = true;
  }

  if (EventCode == 0x02) {
    GPIO::Write(SWITCH_PLUG_PIN_NUM, false);
    Executed = true;
  }

  if (Executed) {
    ESP_LOGI(tag, "Executed. Event code: %s, Operand: %s", Converter::ToHexString(EventCode, 2).c_str(), Converter::ToHexString(Operand, 8).c_str());
    Sensor_t::UpdateSensors();
    return true;
  }

  return false;
}
