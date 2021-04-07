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

#include <string>

#include <Converter.h>

using namespace std;


/*
 * Mode (default - auto):
 * 		0 = off
 * 		1 = auto
 * 		2 = cool
 * 		3 = heat
 * 		4 = dry
 * 		5 = fan_only
 *
 * 	Temperature = Temp from HEX + 16
 *
 * 	Fan (default - auto):
 * 		0 = auto
 * 		1 = low
 * 		2 = mid
 * 		3 = high
 *
 * 	Swing (default = stop)
 * 		0 = stop
 * 		1 = move
 *
 */

class ACOperand {
	public:
		uint16_t		Codeset			= 0x0;
		uint8_t			Mode			= 0x0;
		uint8_t			Temperature		= 0x0;
		uint8_t			FanMode			= 0x0;
		uint8_t			SwingMode		= 0x0;

		ACOperand(uint32_t Operand) {
			Codeset			= Converter::InterpretHexAsDec<uint16_t>((uint16_t)(Operand >> 16));
			Mode			= (uint8_t)((Operand << 16) >> 28);
			Temperature		= (uint8_t)((Operand << 20) >> 28) + 16;
			FanMode  		= (uint8_t)((Operand << 24) >> 28);
			SwingMode 		= (uint8_t)((Operand << 28) >> 28);
		}

		static bool IsSwingSeparateForCodeset(uint16_t CodesetInHEX) {
			if (CodesetInHEX == 0x9015 || CodesetInHEX == 0x9016 || CodesetInHEX == 0x9017 || CodesetInHEX == 0x9019)
				return true;

			return false;
		}

		static bool IsOnSeparateForCodeset(uint16_t CodesetInHEX) {
			if (CodesetInHEX == 0x9000 || CodesetInHEX == 0x9013)
				return true;

			return false;
		}

		string GetQuery() {
			string Result = Converter::ToString<uint16_t>(Codeset);
			while (Result.size() < 4) Result = "0" + Result;
			Result = "operand=" + Result + Converter::ToHexString(Mode, 1) + Converter::ToHexString(Temperature - 16, 1) +  Converter::ToHexString(FanMode, 1) +  Converter::ToHexString(SwingMode,1);
			return Result;
		}

		uint8_t ModeToHomeKitTarget() {
			return ModeToHomeKitTarget(Mode);
		}

		static uint8_t ModeToHomeKitTarget(uint8_t sMode) {
			switch (sMode) {
				case 3:  return 1;
				case 2:  return 2;
				default: return 0;
			}
		}

		uint8_t ModeToHomeKitCurrent() {
			return ModeToHomeKitCurrent(Mode);
		}

		static uint8_t ModeToHomeKitCurrent(uint8_t sMode) {
			switch (sMode) {
				case 3:  return 2;
				case 2:  return 3;
				case 0:  return 0;
				default: return 3;
			}
		}

		uint32_t ToUint32() {
			uint32_t Result = 0x00;

			Result = (uint32_t)Codeset;
			Result = (Result << 4) + (uint8_t)Mode;
			Result = (Result << 4) + (uint8_t)(Temperature - 16);
			Result = (Result << 4) + (uint8_t)FanMode;
			Result = (Result << 4) + (uint8_t)SwingMode;

			return Result;
		}

		uint16_t ToUint16() {
			uint32_t Result = 0x00;

			Result = (Result << 4) + (uint8_t)Mode;
			Result = (Result << 4) + (uint8_t)(Temperature - 16);
			Result = (Result << 4) + (uint8_t)FanMode;
			Result = (Result << 4) + (uint8_t)SwingMode;

			return Result;
		}

};
#endif
