/*
*    CommandIR.cpp
*    CommandBLE_t implementation
*
*/

#include "CommandBLE.h"
#include "BLEServerGeneric.h"

extern BLEServerGeneric_t MyBLEServer;

CommandBLE_t::CommandBLE_t()
{
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

void CommandBLE_t::InitSettings() {
    NVS Memory(NVSCommandsBLEArea);

    if (Memory.IsKeyExists(NVSBLESeqRepeatCounter))
    {
        SequenceRepeatCounter = Memory.GetInt8Bit(NVSBLESeqRepeatCounter);
        if (SequenceRepeatCounter < 1)
            SequenceRepeatCounter = 1;

        IsBLEPairingCodeSkiped = (Memory.GetInt8Bit(NVSBLESkipPairingCode) > 0 ? true : false);
    }
}

bool CommandBLE_t::GetIsBLEPairingCodeSkiped() {
	return IsBLEPairingCodeSkiped;
}

string CommandBLE_t::GetSettings(){
	return "{\"SequenceRepeatCounter\": " +
			Converter::ToString<uint8_t>(SequenceRepeatCounter)
			+ ", \"IsBLEPairingCodeSkiped\":" + (IsBLEPairingCodeSkiped ? "true": "false")  + "}";
}

void CommandBLE_t::SetSettings(WebServer_t::Response &Result, Query_t &Query) {
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

bool CommandBLE_t::Execute(uint8_t EventCode, const char* StringOperand) {
	if (EventCode == 0x01) { // kbd_key
		string Operand(StringOperand);

		if (Operand.size() == 0)
			return false;

#if CONFIG_IDF_TARGET_ESP32
		PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_BALANCE);
#endif

		if (Operand.size() == 4 && Converter::IsStringContainsOnlyDigits(Operand))
		{
			static MediaKeyReport CurrentMediaKeyReport;
			CurrentMediaKeyReport[0] = Converter::UintFromHexString<uint8_t>(Operand.substr(0, 2));
			CurrentMediaKeyReport[1] = Converter::UintFromHexString<uint8_t>(Operand.substr(2, 2));

			MyBLEServer.Write(CurrentMediaKeyReport);

			return true;
		}

		size_t Result = 0;


		vector<string> OperandVector = Converter::StringToVector(Operand, ";");
		uint16_t Delay = 0;

		if (OperandVector.size() == 2) {
			string DelayStr = OperandVector.at(1);
			Delay = Converter::ToUint16(DelayStr);

			Operand = Operand.substr(0, Operand.size() - DelayStr.size() - 1);
			ESP_LOGE("OPERAND", "%s", Operand.c_str());
		}

		if 		(Operand == "MEDIA_NEXT_TRACK") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_NEXT_TRACK, Delay) 		;}
		else if (Operand == "MEDIA_PREV_TRACK") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_PREVIOUS_TRACK, Delay)	;}
		else if (Operand == "MEDIA_STOP") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_STOP, Delay)				;}
		else if (Operand == "MEDIA_PLAY_PAUSE") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_PLAY_PAUSE, Delay)		;}
		else if (Operand == "MEDIA_MUTE") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_MUTE, Delay)				;}
		else if (Operand == "MEDIA_VOLUME_UP") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_VOLUME_UP, Delay)		;}
		else if (Operand == "MEDIA_VOLUME_DOWN") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_VOLUME_DOWN, Delay)		;}
		else if (Operand == "MEDIA_CHANNEL_UP") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_CHANNEL_UP, Delay)		;}
		else if (Operand == "MEDIA_CHANNEL_DOWN") 	{ Result = MyBLEServer.Write(MyBLEServer.KEY_MEDIA_CHANNEL_DOWN, Delay)		;}
		else if (Operand == "CC_POWER") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_POWER, Delay)				;}
		else if (Operand == "CC_SLEEP") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_SLEEP, Delay)				;}
		else if (Operand == "CC_MENU") 				{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU, Delay)				;}
		else if (Operand == "CC_MENU_PICK") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_PICK, Delay)			;}
		else if (Operand == "CC_BACK") 				{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_BACK, Delay)				;}
		else if (Operand == "CC_HOME") 				{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_HOME, Delay)				;}
		else if (Operand == "CC_MENU_UP") 			{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_UP, Delay)				;}
		else if (Operand == "CC_MENU_DOWN") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_DOWN, Delay)			;}
		else if (Operand == "CC_MENU_LEFT") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_LEFT, Delay)			;}
		else if (Operand == "CC_MENU_RIGHT") 		{ Result = MyBLEServer.Write(MyBLEServer.KEY_CC_MENU_RIGHT, Delay)			;}
		else if (Operand == "KEY_ARROW_UP")			{ Result = MyBLEServer.Write(0xDA, Delay)									;}
		else if (Operand == "KEY_ARROW_DOWN")		{ Result = MyBLEServer.Write(0xD9, Delay)									;}
		else if (Operand == "KEY_ARROW_LEFT")		{ Result = MyBLEServer.Write(0xD8, Delay)									;}
		else if (Operand == "KEY_ARROW_RIGHT")		{ Result = MyBLEServer.Write(0xD7, Delay)									;}
		else if (Operand == "KEY_BACKSPACE")		{ Result = MyBLEServer.Write(0xB2, Delay)									;}
		else if (Operand == "KEY_TAB")				{ Result = MyBLEServer.Write(0xB3, Delay)									;}
		else if (Operand == "KEY_RETURN")			{ Result = MyBLEServer.Write(0xB0, Delay)									;}
		else if (Operand == "KEY_ESCAPE")			{ Result = MyBLEServer.Write(0xB1, Delay)									;}
		else if (Operand == "KEY_INSERT")			{ Result = MyBLEServer.Write(0xD1, Delay)									;}
		else if (Operand == "KEY_DELETE")			{ Result = MyBLEServer.Write(0xD4, Delay)									;}
		else if (Operand == "KEY_PAGE_UP")			{ Result = MyBLEServer.Write(0xD3, Delay)									;}
		else if (Operand == "KEY_PAGE_DOWN")		{ Result = MyBLEServer.Write(0xD6, Delay)									;}
		else if (Operand == "KEY_HOME")				{ Result = MyBLEServer.Write(0xD2, Delay)									;}
		else if (Operand == "KEY_END")				{ Result = MyBLEServer.Write(0xD5, Delay)									;}
		else if (Operand == "KEY_CAPS_LOCK")		{ Result = MyBLEServer.Write(0xC1, Delay)									;}
		else if (Operand == "KEY_ENTER")			{ Result = MyBLEServer.Write(0x28, Delay)									;}
		else {
			SendCounter++;
			Result = MyBLEServer.Write(int(StringOperand[0]));
		}

		if (Result > 0)
		{
			Log::Add(Log::Events::Commands::BLEExecuted);
			CommandBLELastKBDSignalSended = StringOperand;
			CommandBLELastTimeSended = ::Time::UptimeU();
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
		if ((::Time::UptimeU() - CommandBLELastTimeSended < Settings.CommandsConfig.BLE.BLEKbdBlockedDelayU)
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
