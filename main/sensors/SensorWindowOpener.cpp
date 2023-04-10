/*
*    SensorWindowOpener.cpp
*    SensorWindowOpener_t implementation
*
*/

#include "SensorWindowOpener.h"

SensorWindowOpener_t::SensorWindowOpener_t() {
	if (GetIsInited()) return;

	ID          = 0x90;
	Name        = "WindowOpener";
	EventCodes  = { 0x00 };

	SetValue(0, "Position");
	SetValue(0, "Mode");

	SetIsInited(true);
}

bool SensorWindowOpener_t::HasPrimaryValue() {
	return false;
}

bool SensorWindowOpener_t::ShouldUpdateInMainLoop() {
	return false;
}

JSON::ValueType SensorWindowOpener_t::ValueType(string Key) {
	if (Key == "Position")
		return JSON::ValueType::Float;

	return JSON::ValueType::String; 
}

string SensorWindowOpener_t::FormatValue(string Key)
{ 
	if (Key == "Mode") {
		switch (GetValue("Mode")) 
		{
			case 0		: return "Basic";
			case 1		: return "Callibration";
			case 2		: return "LimitsEdit";
			default		: return "Basic";
		}
	}
	
	return "";
}

void SensorWindowOpener_t::Update() {
	uint8_t Position = GetValue("Position");

	Wireless.SendBroadcastUpdated(ID, "03", Converter::ToHexString(Position,8));
	Automation.SensorChanged(ID);

	if (IsMatterEnabled())
	{
		//!hap_val_t ValueForPosition;
		//!ValueForPosition.u = Position;

		//!HomeKitUpdateCharValue(0, HAP_SERV_UUID_WINDOW, HAP_CHAR_UUID_CURRENT_POSITION, ValueForPosition);
	}
}

uint32_t SensorWindowOpener_t::ReceiveValue(string Key = "Primary") {
	/*
	if (Key == "Position") {
		CommandWindowOpener_t *WindowOpenerCommand = Command_t::GetCommandByID(ID - 0x80);

		if (Command_t::GetCommandByID(0x10) != nullptr)
			return CommandWindowOpener_t->GetCurrentPos();
	}
	*/

	return 0;
}

bool SensorWindowOpener_t::CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) {
	/*
	uint32_t ValueItem = GetValue();

	if (SceneEventCode == 0x01 && ValueItem == 1) return true;
	if (SceneEventCode == 0x02 && ValueItem == 0) return true;
	*/
	return false;
}
