/* General Operand class for different types of ACs
 * 8 hex digits phrase
 * 1-2 digits: Type of device to be controlled
 * 3-4 digits: Temperature in celcius. if > 128 interpreted as minus,
 * 5: Horizontal swing mode
 * 6: Vertical swing mode
 * 7: Ventillator mode
 * 8: Conditionare mode
*/

#ifndef LIBS_AC_OPERAND_H
#define LIBS_AC_OPERAND_H

#include <stdint.h>
#include "esp_log.h"

class ACOperand {
	public:
		enum GenericMode{ ModeOff 	= 0x0, ModeAuto 	= 0x1, ModeHeat = 0x2, ModeCool = 0x3, ModeDry 		= 0x4, ModeFan 	= 0x5 };
		enum SwingMode	{ SwingOff 	= 0x0, SwingAuto 	= 0x1, SwingHigh= 0x2, SwingLow = 0x3, SwingBreeze 	= 0x4, SwingCirculate = 0x5 };
		enum VentMode	{ VentAuto 	= 0x0, Vent1 		= 0x1, Vent2 	= 0x2, Vent3 	= 0x3, Vent4 		= 0x4, Vent5 	= 0x5, VentQuite = 0x6 };

		uint8_t			DeviceType 		= 0x0;
		uint8_t			Temperature		= 0x0;

		SwingMode		HSwingMode 		= SwingAuto;
		SwingMode		VSwingMode 		= SwingAuto;

		VentMode		VentillatorMode = VentAuto;
		GenericMode		Mode 			= ModeAuto;

		ACOperand(uint32_t Operand = 0x0) {
			if (Operand == 0x0) {
				DeviceType = 0xFF;
				return;
			}

			DeviceType  	= (uint8_t)(Operand >> 24);
			Temperature 	= (uint8_t)((Operand << 8) >> 24);

			HSwingMode		= static_cast<SwingMode>	((Operand << 16) >> 28);
			VSwingMode		= static_cast<SwingMode>	((Operand << 20) >> 28);

			VentillatorMode	= static_cast<VentMode>		((Operand << 24) >> 28);
			Mode			= static_cast<GenericMode>	((Operand << 28) >> 28);
		}

		uint32_t ToUint32() {
			uint32_t Result = 0x00;

			Result = (uint8_t)DeviceType;
			Result = (Result << 8) + Temperature;
			Result = (Result << 4) + (uint8_t)HSwingMode;
			Result = (Result << 4) + (uint8_t)VSwingMode;
			Result = (Result << 4) + (uint8_t)VentillatorMode;
			Result = (Result << 4) + (uint8_t)Mode;

			return Result;
		}
};

#endif
