/*
*    CommandSwitch.h
*    CommandSwitch_t implementation
*
*/

#ifndef COMMANDS_SWITCH
#define COMMANDS_SWITCH

#include "Sensors.h"

class CommandSwitch_t : public Command_t {
  public:
    CommandSwitch_t();
    
    void Overheated() override;

    bool Execute(uint8_t EventCode, const char* StringOperand) override;
};

#endif