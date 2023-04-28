/*
*    CommandColor.h
*    CommandColor_t implementation
*
*/

#ifndef COMMANDS_RGBW
#define COMMANDS_RGBW

#include "Commands.h"

class CommandColor_t : public Command_t {
	public:
		CommandColor_t();

    void Overheated() override;

    bool Execute(uint8_t EventCode, const char* StringOperand) override;
};

#endif