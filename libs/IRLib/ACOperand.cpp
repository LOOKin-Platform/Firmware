/* General Operand class for different types of ACs
 * 8 hex digits phrase
 * 1-2 digits: Type of device to be controlled
 * 3-4 digits: Temperature in celcius. if > 128 interpreted as minus,
 * 5: Horizontal swing mode
 * 6: Vertical swing mode
 * 7: Ventillator mode
 * 8: AC mode
*/

#ifndef LIBS_AC_OPERAND_H
#define LIBS_AC_OPERAND_H

#include <stdint.h>
#include "esp_log.h"

class ACOperand {
	public:
		enum GenericMode{ ModeOff 	= 0x0, ModeAuto 	= 0x1, ModeHeat = 0x2	, ModeCool = 0x3, ModeDry 		= 0x4, ModeFan 	= 0x5 };
		enum SwingMode	{ SwingOff 	= 0x0, SwingAuto 	= 0x1, SwingHigh= 0x2	, SwingLow = 0x3, SwingBreeze 	= 0x4, SwingCirculate = 0x5,
						  SwingTop	= 0x6, SwingMiddle	= 0x7, SwingBottom = 0x8, SwingDown = 0x9 };
		enum FanModeEnum{ FanAuto 	= 0x0, Fan1 		= 0x1, Fan2 	= 0x2	, Fan3 	= 0x3, Fan4 			= 0x4, Fan5 	= 0x5, FanQuite = 0x6 };
		enum CommandEnum{ CommandNULL= 0x0, CommandOff	= 0x1, CommandOn= 0x2	, CommandMode = 0x3, CommandFan = 0x4, CommandTempUp = 0x5, CommandTempDown = 0x6, CommandHSwing = 0x7, CommandVSwing = 0x8  };

		uint8_t			DeviceType 		= 0x0;
		uint8_t			Temperature		= 0x0;

		SwingMode		HSwingMode 		= SwingAuto;
		SwingMode		VSwingMode 		= SwingAuto;

		FanModeEnum		FanMode 		= FanAuto;
		GenericMode		Mode 			= ModeAuto;

		CommandEnum		Command			= CommandNULL;

		ACOperand(uint32_t Operand = 0x0, uint8_t Misc = 0x0) {
			if (Misc != 0x0) { // fix Operand & Misc content for AC
				uint32_t Temp = Misc;
				Misc = (uint8_t)((Operand << 24) >> 24);
				Operand = (Operand >> 8) + (Temp << 24);

				if (Misc > 0 && Misc < 9)
					Command = static_cast<CommandEnum>(Misc);
			}

			if (Operand == 0x0) {
				DeviceType = 0xFF;
				return;
			}

			DeviceType  	= (uint8_t)(Operand >> 24);
			Temperature 	= (uint8_t)((Operand << 8) >> 24);

			HSwingMode		= static_cast<SwingMode>	((Operand << 16) >> 28);
			VSwingMode		= static_cast<SwingMode>	((Operand << 20) >> 28);

			FanMode			= static_cast<FanModeEnum>	((Operand << 24) >> 28);
			Mode			= static_cast<GenericMode>	((Operand << 28) >> 28);
		}

		uint32_t ToUint32() {
			uint32_t Result = 0x00;

			Result = (uint8_t)DeviceType;
			Result = (Result << 8) + Temperature;
			Result = (Result << 4) + (uint8_t)HSwingMode;
			Result = (Result << 4) + (uint8_t)VSwingMode;
			Result = (Result << 4) + (uint8_t)FanMode;
			Result = (Result << 4) + (uint8_t)Mode;

			return Result;
		}
};

#endif
