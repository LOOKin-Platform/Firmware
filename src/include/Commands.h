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

#include "Query.h"
#include "WebServer.h"

#include "JSON.h"
#include "HardwareIO.h"

#include "Convert.h"

using namespace std;

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

class CommandColor_t : public Command_t {
  public:
    CommandColor_t();
    bool Execute(uint8_t EventCode, uint32_t Operand) override;

  private:
    gpio_num_t      GPIORed, GPIOGreen, GPIOBlue, GPIOWhite;
    ledc_timer_t    TimerIndex;
    ledc_channel_t  PWMChannelRED, PWMChannelGreen, PWMChannelBlue, PWMChannelWhite;
};


#endif
