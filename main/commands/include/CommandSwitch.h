/*
*    CommandSwitch.h
*    CommandSwitch_t implementation
*
*/

#ifndef COMMANDS_SWITCH
#define COMMANDS_SWITCH

#include "Commands.h"


#define	NVSCommandsSwitchArea   "CommandSwitch"
#define NVSSwitchSaveStatus 	  "SaveStatus"
#define NVSSwitchLastStatus 	  "LastStatus"

class CommandSwitch_t : public Command_t {
  public:
    CommandSwitch_t();
    
    void    Overheated() override;

    bool    Execute(uint8_t EventCode, const char* StringOperand) override;

    void    InitSettings() override;
    string  GetSettings() override;
    void    SetSettings(WebServer_t::Response &Result, Query_t &Query) override;

    bool    SaveStatusEnabled();
    void    SaveStatus(bool);
    bool    GetStatus();
};

#endif