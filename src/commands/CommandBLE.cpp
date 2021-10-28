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

				if 		(Operand == "KM_NEXT_TRACK")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_NEXT_TRACK)			; return true;}
				else if (Operand == "KM_PREV_TRACK")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_PREVIOUS_TRACK)		; return true;}
				else if (Operand == "KM_STOP")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_STOP)					; return true;}
				else if (Operand == "KM_PLAY_PAUSE")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_PLAY_PAUSE)			; return true;}
				else if (Operand == "KM_MUTE")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_MUTE)					; return true;}
				else if (Operand == "KM_VOLUME_UP")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_VOLUME_UP)			; return true;}
				else if (Operand == "KM_VOLUME_DOWN")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_VOLUME_DOWN)			; return true;}
				else if (Operand == "KM_WWW_HOME")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_HOME)				; return true;}
				else if (Operand == "KM_LOCAL_MACHINE_BROWSER")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_LOCAL_MACHINE_BROWSER); return true;}
				else if (Operand == "KM_CALCULATOR")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_CALCULATOR)			; return true;}
				else if (Operand == "KM_WWW_BOOKMARKS")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_BOOKMARKS)		; return true;}
				else if (Operand == "KM_WWW_SEARCH")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_SEARCH)			; return true;}
				else if (Operand == "KM_WWW_STOP")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_STOP)				; return true;}
				else if (Operand == "KM_WWW_BACK")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_WWW_BACK)				; return true;}
				else if (Operand == "KM_CC_CONFIGURATION")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION); return true;}
				else if (Operand == "KM_EMAIL_READER")
						{ BLEServer.Write(BLEServer.KEY_MEDIA_EMAIL_READER)			; return true;}

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

		static void ACReadStarted(const char *IP) {
			RMT::TXClear();

			IRACFrequency = 38000;
			ACCode.clear();
			ACReadPart = "";
		}
};

#endif
