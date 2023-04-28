/*
*    CommandWindowOpener_t.h
*    CommandWindowOpener_t implementation
*
*/

#ifndef COMMANDS_WINDOWOPENER
#define COMMANDS_WINDOWOPENER

#include "Commands.h"

class CommandWindowOpener_t : public Command_t {
  public:
	inline static uint8_t 	CurrentOpenProcent 				= 0;		// Текущий процент открытия 0-100%
	inline static bool 		IsLimitsModifficationEnabled	= false;	// Флаг режима изменения крайних положений
	static inline uint8_t	LastPosition = 0;

	enum OperationalModeEnum { Basic = 0x0, Callibration = 0x1, LimitsEdit = 0x2 };

	CommandWindowOpener_t();
    
    void Overheated() override;

	// Команда открытия окна на нужную позицию
	virtual void 	SetPosition(uint8_t Target) 			{ };
	
	virtual void 	SetLimitModeEnabledPostActions()		{ };
	virtual bool 	ResetMotorPosition()					{ return false; };
	virtual bool 	SaveOpenPosition()						{ return false; };
	virtual bool 	SaveClosePosition()						{ return false; };
	virtual bool 	AutoCallibrationStart()					{ return false; };

	void 			SetCurrentOpenPercent(uint8_t Value);
	void 			SetCurrentOpenFinished(uint8_t Value);

	void 			UpdateSensorWindowOpener(uint8_t Value, bool ForceUpdate  = false);

	void 			SetCurrentMode(OperationalModeEnum Mode);
	bool 			Execute(uint8_t EventCode, const char* StringOperand) override;
};

#endif