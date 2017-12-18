/*
*    Commands.cpp
*    Class to handle API /Commands
*
*/
#include "Globals.h"
#include "Commands.h"

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
  for (auto& Command : Commands)
    if (Converter::ToLower(Command->Name) == Converter::ToLower(CommandName))
      return Command;

  return nullptr;
}

Command_t* Command_t::GetCommandByID(uint8_t CommandID) {
  for (auto& Command : Commands)
    if (Command->ID == CommandID)
      return Command;

  return nullptr;
}

vector<Command_t*> Command_t::GetCommandsForDevice() {
  vector<Command_t*> Commands = {};

  switch (Device->Type->Hex) {
    case DEVICE_TYPE_PLUG_HEX:
      Commands = { new CommandSwitch_t(), new CommandColor_t() };
      break;
  }

  return Commands;
}

void Command_t::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
    // Вывести список всех комманд
    if (URLParts.size() == 0 && Type == QueryType::GET) {

      vector<string> Vector = vector<string>();
      for (auto& Command : Commands)
        Vector.push_back(Command->Name);

      Result.Body = JSON::CreateStringFromVector(Vector);
    }

    // Запрос списка действий конкретной команды
    if (URLParts.size() == 1 && Type == QueryType::GET) {
      Command_t* Command = Command_t::GetCommandByName(URLParts[0]);

      if (Command!=nullptr)
        if (Command->Events.size() > 0) {

          vector<string> Vector = vector<string>();
          for (auto& Event: Command->Events)
            Vector.push_back(Event.first);

          Result.Body = JSON::CreateStringFromVector(Vector);
        }
    }

    /*
    GET /switch/on
    GET /switch/on?operand=FFFFFF
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
            Result.SetSuccess();
          else
            Result.SetFail();
        }
    }

    /*
    GET /switch/on/FFFFFF
    POST /switch/on/FFFFFF
    */
    if (URLParts.size() == 3 && Type != QueryType::DELETE) {
      string CommandName = URLParts[0];
      string Action = URLParts[1];
      string Operand = URLParts[2];

      Command_t* Command = Command_t::GetCommandByName(CommandName);
      if (Command == nullptr)
        Command = Command_t::GetCommandByID(Converter::UintFromHexString<uint8_t>(CommandName));

      if (Command != nullptr) {
          if (Command->Execute(Command->GetEventCode(Action), Converter::UintFromHexString<uint32_t>(Operand)))
            Result.SetSuccess();
          else
            Result.SetFail();
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

    switch (Device->Type->Hex) {
      case DEVICE_TYPE_PLUG_HEX: GPIO::Setup(SWITCH_PLUG_PIN_NUM); break;
    }
}

void CommandSwitch_t::Overheated() {
  Execute(0x02, 0);
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
    Sensor_t::GetSensorByID(ID + 0x80)->Update();
    return true;
  }

  return false;
}

/************************************/
/*          Color Command           */
/************************************/

CommandColor_t::CommandColor_t() {
    ID          = 0x04;
    Name        = "RGBW";

    Events["color"]       = 0x01;
    Events["brightness"]  = 0x02;

    switch (Device->Type->Hex) {
      case DEVICE_TYPE_PLUG_HEX:
        TimerIndex  = COLOR_PLUG_TIMER_INDEX;

        GPIORed     = COLOR_PLUG_RED_PIN_NUM;
        GPIOGreen   = COLOR_PLUG_GREEN_PIN_NUM;
        GPIOBlue    = COLOR_PLUG_BLUE_PIN_NUM;
        GPIOWhite   = COLOR_PLUG_WHITE_PIN_NUM;

        PWMChannelRED   = COLOR_PLUG_RED_PWMCHANNEL;
        PWMChannelGreen = COLOR_PLUG_GREEN_PWMCHANNEL;
        PWMChannelBlue  = COLOR_PLUG_BLUE_PWMCHANNEL;
        PWMChannelWhite = COLOR_PLUG_WHITE_PWMCHANNEL;
        break;
    }

    GPIO::SetupPWM(GPIORed  , TimerIndex, PWMChannelRED);
    GPIO::SetupPWM(GPIOGreen, TimerIndex, PWMChannelGreen);
    GPIO::SetupPWM(GPIOBlue , TimerIndex, PWMChannelBlue);
    GPIO::SetupPWM(GPIOWhite, TimerIndex, PWMChannelWhite);
}

void CommandColor_t::Overheated() {
  Execute(0x01, 0x00000000);
}

bool CommandColor_t::Execute(uint8_t EventCode, uint32_t Operand) {
  bool Executed = false;

  if (EventCode == 0x01) {

    uint8_t Red, Green, Blue = 0;

    Blue    = Operand&0x000000FF;
    Green   = (Operand&0x0000FF00)>>8;
    Red     = (Operand&0x00FF0000)>>16;

    //(Operand&0xFF000000)>>24;
    //floor((Red  * Power) / 255)

    if ((Blue == Green && Green == Red) && GPIOWhite != GPIO_NUM_0)
      GPIO::PWMFadeTo(PWMChannelWhite, Blue);
    else {
      if (GPIORed   != GPIO_NUM_0) GPIO::PWMFadeTo(PWMChannelRED  , Red);
      if (GPIOGreen != GPIO_NUM_0) GPIO::PWMFadeTo(PWMChannelGreen, Green);
      if (GPIOBlue  != GPIO_NUM_0) GPIO::PWMFadeTo(PWMChannelBlue , Blue);
    }

    Executed = true;
  }

  if (EventCode == 0x02)
    Executed = true;

  if (Executed) {
    ESP_LOGI(tag, "Executed. Event code: %s, Operand: %s", Converter::ToHexString(EventCode, 2).c_str(), Converter::ToHexString(Operand, 8).c_str());
    Sensor_t::GetSensorByID(ID + 0x80)->Update();
    return true;
  }

  return false;
}
