/*
*    CommandSwitch.cpp
*    CommandSwitch_t implementation
*
*/

#include "Sensors.h"
#include "CommandSwitch.h"

#include "HardwareIO.h"

CommandSwitch_t::CommandSwitch_t() {
	ID          = 0x01;
	Name        = "Switch";

	Events["on"]  = 0x01;
	Events["off"] = 0x02;

	if (Settings.GPIOData.GetCurrent().Switch.GPIO != GPIO_NUM_0)
		GPIO::Setup(Settings.GPIOData.GetCurrent().Switch.GPIO);
}

void CommandSwitch_t::Overheated() {
		Execute(0x02, 0);
}

bool CommandSwitch_t::Execute(uint8_t EventCode, const char* StringOperand) {
	bool Executed = false;

	if (EventCode == 0x00)
		return false;

	if (EventCode == 0x01) {
		GPIO::Write(Settings.GPIOData.GetCurrent().Switch.GPIO, true);
		Executed = true;
	}

	if (EventCode == 0x02) {
		GPIO::Write(Settings.GPIOData.GetCurrent().Switch.GPIO, false);
		Executed = true;
	}

	if (Executed) {
		//ESP_LOGI(tag, "Executed. Event code: %s, Operand: %s", Converter::ToHexString(EventCode, 2).c_str(), Converter::ToHexString(Operand, 8).c_str());
		if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr)
			Sensor_t::GetSensorByID(ID + 0x80)->Update();

		SaveStatus(EventCode == 0x1);

		return true;
	}

	return false;
}

void CommandSwitch_t::InitSettings() {
    NVS Memory(NVSCommandsSwitchArea);

    if (!Memory.IsKeyExists(NVSSwitchSaveStatus))
		Memory.SetInt8Bit(NVSSwitchSaveStatus, 1);
	else 
	{
		if (SaveStatusEnabled()) {
			uint8_t EventID = (GetStatus() == 0) ? 2 : 1;
			ESP_LOGE("EVENTID", "%d", EventID);
			if (EventID == 1) { // нужно включить при запуске
				Execute(EventID,"");
			}
		}
	}
}

string CommandSwitch_t::GetSettings(){
	string SaveStatusString = SaveStatusEnabled() ? "true": "false";
	string LastStatusString = GetStatus() ? "1" : "0";
	return  
	"{\"SaveStatus\": " + SaveStatusString + ", \"LastStatus:\":" + LastStatusString + "}";
}

void CommandSwitch_t::SetSettings(WebServer_t::Response &Result, Query_t &Query) {
	JSON JSONItem(Query.GetBody());

	if (JSONItem.GetKeys().size() == 0)
	{
		Result.SetInvalid();
		return;
	}

	bool IsChanged = false;

	NVS Memory(NVSCommandsSwitchArea);

	if (JSONItem.IsItemExists("SaveStatus") && JSONItem.IsItemBool("SaveStatus"))
	{
		bool SaveStatusFlag = JSONItem.GetBoolItem(NVSSwitchSaveStatus);
		Memory.SetInt8Bit(NVSSwitchSaveStatus, (SaveStatusFlag) ? 1 : 0);
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

bool CommandSwitch_t::SaveStatusEnabled() {
	NVS Memory(NVSCommandsSwitchArea);
	return (Memory.GetInt8Bit(NVSSwitchSaveStatus) == 0) ? false : true;
}

void CommandSwitch_t::SaveStatus(bool Status) {
	ESP_LOGE("CommandSwitch_t::SaveStatus", "%d", (Status) ? 1 : 0);
	
	if (SaveStatusEnabled()) 
	{
		ESP_LOGE("CommandSwitch_t::SaveStatus ->>","!");

		NVS Memory(NVSCommandsSwitchArea);
		return (Memory.SetInt8Bit(NVSSwitchLastStatus, Status ? 1 : 0));
		Memory.Commit();
	}
}

bool CommandSwitch_t::GetStatus() {
	NVS Memory(NVSCommandsSwitchArea);
	return (Memory.GetInt8Bit(NVSSwitchLastStatus) == 0) ? false : true;
}