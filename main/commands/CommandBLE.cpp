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

extern BLEServer_t 		MyBLEServer;

static string 	CommandBLELastKBDSignalSended 	= "";
static uint64_t	CommandBLELastTimeSended		= 0;

#define	NVSCommandsBLEArea 		"CommandBLE"
#define NVSBLESeqRepeatCounter 	"BLERepeatCount"
#define NVSBLESkipPairingCode 	"BLECodeSkiped"

class CommandBLE_t : public Command_t {
	private:
		uint8_t 		SequenceRepeatCounter 	= 5;
		bool			IsBLEPairingCodeSkiped	= false;

	public:
		uint32_t		SendCounter 		= 0;


		CommandBLE_t() {
			ID          				= 0x08;
			Name        				= "BLE";

			Events["kbd_key"]			= 0x01;
			Events["kbd_keydown"]		= 0x02;
			Events["kbd_keyup"]			= 0x03;
			Events["kbd_key_repeat"]	= 0x04;

			Events["kbd_key-blocked"]	= 0x90;
			Events["kbd_key-repeated"]	= 0x91;
			Events["pairing_pin"]		= 0xFF;
		}

		void InitSettings() override {
			NVS Memory(NVSCommandsBLEArea);

			if (Memory.IsKeyExists(NVSBLESeqRepeatCounter))
			{
				SequenceRepeatCounter = Memory.GetInt8Bit(NVSBLESeqRepeatCounter);
				if (SequenceRepeatCounter < 1)
					SequenceRepeatCounter = 1;

				IsBLEPairingCodeSkiped = (Memory.GetInt8Bit(NVSBLESkipPairingCode) > 0 ? true : false);
			}
		}

		bool GetIsBLEPairingCodeSkiped() const {
			return IsBLEPairingCodeSkiped;
		}

		string GetSettings() override {
			return "{\"SequenceRepeatCounter\": " +
					Converter::ToString<uint8_t>(SequenceRepeatCounter)
					+ ", \"IsBLEPairingCodeSkiped\":" + (IsBLEPairingCodeSkiped ? "true": "false")  + "}";
		}

		void SetSettings(WebServer_t::Response &Result, Query_t &Query) override {
			JSON JSONItem(Query.GetBody());

			if (JSONItem.GetKeys().size() == 0)
			{
				Result.SetInvalid();
				return;
			}

			bool IsChanged = false;

			NVS Memory(NVSCommandsBLEArea);

			if (JSONItem.IsItemExists("SequenceRepeatCounter") && JSONItem.IsItemNumber("SequenceRepeatCounter"))
			{
				SequenceRepeatCounter = JSONItem.GetIntItem("SequenceRepeatCounter");
				Memory.SetInt8Bit(NVSBLESeqRepeatCounter, SequenceRepeatCounter);
				IsChanged = true;
			}

			if (JSONItem.IsItemExists("IsBLEPairingCodeSkiped") && JSONItem.IsItemBool("IsBLEPairingCodeSkiped"))
			{
				IsBLEPairingCodeSkiped = JSONItem.GetBoolItem("IsBLEPairingCodeSkiped");
				Memory.SetInt8Bit(NVSBLESkipPairingCode, IsBLEPairingCodeSkiped ? 1 : 0);
				IsChanged = true;
			}

			if (IsChanged)
			{
				Memory.Commit();
				Result.SetSuccess();
			}
			else
				Result.SetInvalid();
		}

		bool Execute(uint8_t EventCode, const char* StringOperand) override {
			if (EventCode == 0x01) { // kbd_key
				string Operand(StringOperand);

				if (Operand.size() == 0)
					return false;

				PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_BALANCE);

				if (Operand.size() == 4 && Converter::IsStringContainsOnlyDigits(Operand))
				{
					static MediaKeyReport CurrentMediaKeyReport;
					CurrentMediaKeyReport[0] = Converter::UintFromHexString<uint8_t>(Operand.substr(0, 2));
					CurrentMediaKeyReport[1] = Converter::UintFromHexString<uint8_t>(Operand.substr(2, 2));

					MyBLEServer.Write(CurrentMediaKeyReport);

					return true;
				}

				size_t Result = 0;

				if 		(Operand == "MEDIA_NEXT_TRACK") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_NEXT_TRACK) 			;}
				else if (Operand == "MEDIA_PREV_TRACK") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_PREVIOUS_TRACK)		;}
				else if (Operand == "MEDIA_STOP") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_STOP)				;}
				else if (Operand == "MEDIA_PLAY_PAUSE") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_PLAY_PAUSE)			;}
				else if (Operand == "MEDIA_MUTE") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_MUTE)				;}
				else if (Operand == "MEDIA_VOLUME_UP") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_VOLUME_UP)			;}
				else if (Operand == "MEDIA_VOLUME_DOWN") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_VOLUME_DOWN)			;}
				else if (Operand == "MEDIA_CHANNEL_UP") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_CHANNEL_UP)			;}
				else if (Operand == "MEDIA_CHANNEL_DOWN") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_CHANNEL_DOWN)		;}
				else if (Operand == "CC_POWER") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_POWER)					;}
				else if (Operand == "CC_SLEEP") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_SLEEP)					;}
				else if (Operand == "CC_MENU") 				{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU)					;}
				else if (Operand == "CC_MENU_PICK") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_PICK)				;}
				else if (Operand == "CC_BACK") 				{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_BACK)					;}
				else if (Operand == "CC_HOME") 				{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_HOME)					;}
				else if (Operand == "CC_MENU_UP") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_UP)				;}
				else if (Operand == "CC_MENU_DOWN") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_DOWN)				;}
				else if (Operand == "CC_MENU_LEFT") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_LEFT)				;}
				else if (Operand == "CC_MENU_RIGHT") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_RIGHT)				;}
				else if (Operand == "KEY_ARROW_UP")			{ Result = MyBLEServer.Write(0xDA)									;}
				else if (Operand == "KEY_ARROW_DOWN")		{ Result = MyBLEServer.Write(0xD9)									;}
				else if (Operand == "KEY_ARROW_LEFT")		{ Result = MyBLEServer.Write(0xD8)									;}
				else if (Operand == "KEY_ARROW_RIGHT")		{ Result = MyBLEServer.Write(0xD7)									;}
				else if (Operand == "KEY_BACKSPACE")		{ Result = MyBLEServer.Write(0xB2)									;}
				else if (Operand == "KEY_TAB")				{ Result = MyBLEServer.Write(0xB3)									;}
				else if (Operand == "KEY_RETURN")			{ Result = MyBLEServer.Write(0xB0)									;}
				else if (Operand == "KEY_ESCAPE")			{ Result = MyBLEServer.Write(0xB1)									;}
				else if (Operand == "KEY_INSERT")			{ Result = MyBLEServer.Write(0xD1)									;}
				else if (Operand == "KEY_DELETE")			{ Result = MyBLEServer.Write(0xD4)									;}
				else if (Operand == "KEY_PAGE_UP")			{ Result = MyBLEServer.Write(0xD3)									;}
				else if (Operand == "KEY_PAGE_DOWN")		{ Result = MyBLEServer.Write(0xD6)									;}
				else if (Operand == "KEY_HOME")				{ Result = MyBLEServer.Write(0xD2)									;}
				else if (Operand == "KEY_END")				{ Result = MyBLEServer.Write(0xD5)									;}
				else if (Operand == "KEY_CAPS_LOCK")		{ Result = MyBLEServer.Write(0xC1)									;}
				else if (Operand == "KEY_ENTER")			{ Result = MyBLEServer.Write(0x28)									;}
				else {
					SendCounter++;
					Result = MyBLEServer.Write(int(StringOperand[0]));
				}

				if (Result > 0)
				{
					Log::Add(Log::Events::Commands::BLEExecuted);
					CommandBLELastKBDSignalSended = StringOperand;
					CommandBLELastTimeSended = Time::UptimeU();
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

			if (EventCode == 0x90) { // Blocked kbd_key
				if ((Time::UptimeU() - CommandBLELastTimeSended < Settings.CommandsConfig.BLE.BLEKbdBlockedDelayU)
					&& (StringOperand == CommandBLELastKBDSignalSended))
					return false;

				return Execute(0x01, StringOperand);
			}

			if (EventCode == 0x91) { // Repeated kbd_key
				bool IsExecuted = false;

				for (int i=0; i < SequenceRepeatCounter; i++) {
					bool IsOK = Execute(0x01, StringOperand);

					if (IsOK && !IsExecuted)
						IsExecuted = IsOK;
				}

				return IsExecuted;
			}

			if (EventCode == 0xFF) { // pincode entered
				string Operand(StringOperand);
				uint32_t PairingPin = Converter::ToUint32(Operand);

				MyBLEServer.SetPairingPin(PairingPin);
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
