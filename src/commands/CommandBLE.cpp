/*
*    CommandIR.cpp
*    CommandBLE_t implementation
*
*/
#ifndef COMMANDS_BLE
#define COMMANDS_BLE

#include "Sensors.h"
#include "BLEServer.h"

#include "Data.h"

extern DataEndpoint_t 	*Data;
extern BLEServer_t		BLEServer;

class CommandBLE_t : public Command_t {
	public:
		CommandBLE_t() {
			ID          				= 0x08;
			Name        				= "BLE";

			Events["kbd_key"]			= 0x01;
			Events["kbd_keydown"]		= 0x02;
			Events["kbd_keyup"]			= 0x03;
		}

		bool Execute(uint8_t EventCode, const char* StringOperand) override {
			if (EventCode == 0x01) { // kbd_key
				string Operand(StringOperand);

				if (Operand.size() == 0)
					return false;

				if 		(Operand == "MEDIA_NEXT_TRACK") 	{ BLEServer.Write(BLEServer.KEY_MEDIA_NEXT_TRACK) 			; return true;}
				else if (Operand == "MEDIA_PREV_TRACK") 	{ BLEServer.Write(BLEServer.KEY_MEDIA_PREVIOUS_TRACK)		; return true;}
				else if (Operand == "MEDIA_STOP") 			{ BLEServer.Write(BLEServer.KEY_MEDIA_STOP)					; return true;}
				else if (Operand == "MEDIA_PLAY_PAUSE") 	{ BLEServer.Write(BLEServer.KEY_MEDIA_PLAY_PAUSE)			; return true;}
				else if (Operand == "MEDIA_MUTE") 			{ BLEServer.Write(BLEServer.KEY_MEDIA_MUTE)					; return true;}
				else if (Operand == "MEDIA_VOLUME_UP") 		{ BLEServer.Write(BLEServer.KEY_MEDIA_VOLUME_UP)			; return true;}
				else if (Operand == "MEDIA_VOLUME_DOWN") 	{ BLEServer.Write(BLEServer.KEY_MEDIA_VOLUME_DOWN)			; return true;}
				else if (Operand == "MEDIA_WWW_HOME") 		{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_HOME)				; return true;}
				else if (Operand == "MEDIA_BROWSER") 		{ BLEServer.Write(BLEServer.KEY_MEDIA_LOCAL_MACHINE_BROWSER); return true;}
				else if (Operand == "MEDIA_CALCULATOR")		{ BLEServer.Write(BLEServer.KEY_MEDIA_CALCULATOR)			; return true;}
				else if (Operand == "MEDIA_WWW_BOOKMARKS")	{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_BOOKMARKS)		; return true;}
				else if (Operand == "MEDIA_WWW_SEARCH")		{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_SEARCH)			; return true;}
				else if (Operand == "MEDIA_WWW_STOP") 		{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_STOP)				; return true;}
				else if (Operand == "MEDIA_WWW_BACK")		{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_BACK)				; return true;}
				else if (Operand == "MEDIA_CONFIGURATION")	{ BLEServer.Write(BLEServer.KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION); return true;}
				else if (Operand == "MEDIA_EMAIL_READER")	{ BLEServer.Write(BLEServer.KEY_MEDIA_EMAIL_READER)			; return true;}
				else if (Operand == "KEY_ARROW_UP")			{ BLEServer.Write(0xDA)										; return true;}
				else if (Operand == "KEY_ARROW_DOWN")		{ BLEServer.Write(0xD9)										; return true;}
				else if (Operand == "KEY_ARROW_LEFT")		{ BLEServer.Write(0xD8)										; return true;}
				else if (Operand == "KEY_ARROW_RIGHT")		{ BLEServer.Write(0xD7)										; return true;}
				else if (Operand == "KEY_BACKSPACE")		{ BLEServer.Write(0xB2)										; return true;}
				else if (Operand == "KEY_TAB")				{ BLEServer.Write(0xB3)										; return true;}
				else if (Operand == "KEY_RETURN")			{ BLEServer.Write(0xB0)										; return true;}
				else if (Operand == "KEY_ESCAPE")			{ BLEServer.Write(0xB1)										; return true;}
				else if (Operand == "KEY_INSERT")			{ BLEServer.Write(0xD1)										; return true;}
				else if (Operand == "KEY_DELETE")			{ BLEServer.Write(0xD4)										; return true;}
				else if (Operand == "KEY_PAGE_UP")			{ BLEServer.Write(0xD3)										; return true;}
				else if (Operand == "KEY_PAGE_DOWN")		{ BLEServer.Write(0xD6)										; return true;}
				else if (Operand == "KEY_HOME")				{ BLEServer.Write(0xD2)										; return true;}
				else if (Operand == "KEY_END")				{ BLEServer.Write(0xD5)										; return true;}
				else if (Operand == "KEY_CAPS_LOCK")		{ BLEServer.Write(0xC1)										; return true;}
				else if (Operand == "KEY_ENTER")			{ BLEServer.Write(0x28)										; return true;}

				ESP_LOGE("StringOperand", "%s %d", StringOperand, int(StringOperand[0]));

				BLEServer.Write(int(StringOperand[0]));
				return true;
			}

			return false;
		}

		/*
		void TriggerStateChange(IRLib &Signal) {
			if (Settings.eFuse.Type == Settings.Devices.Remote)
				((DataRemote_t*)Data)->SetExternalStatusByIRCommand(Signal);
		}
		*/

		// AC Codeset callbacks
};

#endif
