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

static string 	CommandBLELastKBDSignalSended 	= "";


class CommandBLE_t : public Command_t {
	public:
		uint32_t	SendCounter 		= 0;


		CommandBLE_t() {
			ID          				= 0x08;
			Name        				= "BLE";

			Events["kbd_key"]			= 0x01;
			Events["kbd_keydown"]		= 0x02;
			Events["kbd_keyup"]			= 0x03;
			Events["kbd_key_repeat"]	= 0x04;
		}

		bool Execute(uint8_t EventCode, const char* StringOperand) override {
			if (EventCode == 0x01) { // kbd_key
				string Operand(StringOperand);

				if (Operand.size() == 0)
					return false;

				size_t Result = 0;

				if 		(Operand == "MEDIA_NEXT_TRACK") 	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_NEXT_TRACK) 			;}
				else if (Operand == "MEDIA_PREV_TRACK") 	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_PREVIOUS_TRACK)		;}
				else if (Operand == "MEDIA_STOP") 			{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_STOP)				;}
				else if (Operand == "MEDIA_PLAY_PAUSE") 	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_PLAY_PAUSE)			;}
				else if (Operand == "MEDIA_MUTE") 			{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_MUTE)				;}
				else if (Operand == "MEDIA_VOLUME_UP") 		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_VOLUME_UP)			;}
				else if (Operand == "MEDIA_VOLUME_DOWN") 	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_VOLUME_DOWN)			;}
				else if (Operand == "MEDIA_WWW_HOME") 		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_WWW_HOME)			;}
				else if (Operand == "MEDIA_BROWSER") 		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_LOCAL_MACHINE_BROWSER);}
				else if (Operand == "MEDIA_CALCULATOR")		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_CALCULATOR)			;}
				else if (Operand == "MEDIA_WWW_BOOKMARKS")	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_WWW_BOOKMARKS)		;}
				else if (Operand == "MEDIA_WWW_SEARCH")		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_WWW_SEARCH)			;}
				else if (Operand == "MEDIA_WWW_STOP") 		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_WWW_STOP)			;}
				else if (Operand == "MEDIA_WWW_BACK")		{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_WWW_BACK)			;}
				else if (Operand == "MEDIA_CONFIGURATION")	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_CC_CONFIGURATION)	;}
				else if (Operand == "MEDIA_EMAIL_READER")	{ Result = BLEServer.Write(BLEServer.KEY_MEDIA_EMAIL_READER)		;}
				else if (Operand == "KEY_ARROW_UP")			{ Result = BLEServer.Write(0xDA)									;}
				else if (Operand == "KEY_ARROW_DOWN")		{ Result = BLEServer.Write(0xD9)									;}
				else if (Operand == "KEY_ARROW_LEFT")		{ Result = BLEServer.Write(0xD8)									;}
				else if (Operand == "KEY_ARROW_RIGHT")		{ Result = BLEServer.Write(0xD7)									;}
				else if (Operand == "KEY_BACKSPACE")		{ Result = BLEServer.Write(0xB2)									;}
				else if (Operand == "KEY_TAB")				{ Result = BLEServer.Write(0xB3)									;}
				else if (Operand == "KEY_RETURN")			{ Result = BLEServer.Write(0xB0)									;}
				else if (Operand == "KEY_ESCAPE")			{ Result = BLEServer.Write(0xB1)									;}
				else if (Operand == "KEY_INSERT")			{ Result = BLEServer.Write(0xD1)									;}
				else if (Operand == "KEY_DELETE")			{ Result = BLEServer.Write(0xD4)									;}
				else if (Operand == "KEY_PAGE_UP")			{ Result = BLEServer.Write(0xD3)									;}
				else if (Operand == "KEY_PAGE_DOWN")		{ Result = BLEServer.Write(0xD6)									;}
				else if (Operand == "KEY_HOME")				{ Result = BLEServer.Write(0xD2)									;}
				else if (Operand == "KEY_END")				{ Result = BLEServer.Write(0xD5)									;}
				else if (Operand == "KEY_CAPS_LOCK")		{ Result = BLEServer.Write(0xC1)									;}
				else if (Operand == "KEY_ENTER")			{ Result = BLEServer.Write(0x28)									;}
				else {
					SendCounter++;
					Result = BLEServer.Write(int(StringOperand[0]));
				}

				if (Result > 0)
				{
					Log::Add(Log::Events::Commands::BLEExecuted);
					CommandBLELastKBDSignalSended = StringOperand;
				}
				else
					Log::Add(Log::Events::Commands::BLEFailed);

				return (Result > 0);
			}

			if (EventCode == 0x04)
			{
				if (CommandBLELastKBDSignalSended != "")
					return Execute(0x01, CommandBLELastKBDSignalSended.c_str());

				return false;
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
