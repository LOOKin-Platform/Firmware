#include "HomeKit.h"

const char *Tag = "HAP Bridge";

TaskHandle_t 		HomeKit::TaskHandle = NULL;

bool 				HomeKit::IsRunning 		= false;
bool 				HomeKit::IsAP 			= false;
HomeKit::ModeEnum	HomeKit::Mode			= HomeKit::ModeEnum::NONE;

uint64_t			HomeKit::TVHIDLastUpdated = 0;

vector<hap_acc_t*> 	HomeKit::BridgedAccessories = vector<hap_acc_t*>();

HomeKit::AccessoryData_t::AccessoryData_t(string sName, string sModel, string sID) {
	//sName.copy(Name, sName.size(), 0);

	Converter::FindAndRemove(sName, "., ");

	if (sName.size() > 32) sName 	= sName.substr(0, 32);
	if (sModel.size() > 2) sModel 	= sModel.substr(0, 32);
	if (sID.size() > 8) 	sID 	= sID.substr(0, 32);

	::sprintf(Name	, sName.c_str());
	::sprintf(Model	, sModel.c_str());
	::sprintf(ID	, sID.c_str());
}

void HomeKit::WiFiSetMode(bool sIsAP, string sSSID, string sPassword) {
	IsAP 		= sIsAP;
}

void HomeKit::Init() {
	NVS Memory(NVS_HOMEKIT_AREA);
	uint8_t LoadedMode = Memory.GetInt8Bit(NVS_HOMEKIT_AREA_MODE);

	uint8_t NewMode = 0;

	if (LoadedMode == 0) { // check and set if there first start of new device
		if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1)
			NewMode = 1;

		if (LoadedMode != NewMode) {
			Memory.SetInt8Bit(NVS_HOMEKIT_AREA_MODE, NewMode);
			LoadedMode = NewMode;
		}
	}

	Mode = static_cast<ModeEnum>(LoadedMode);

	if (Mode != ModeEnum::NONE)
		HomeKit::Start();
	else
		WebServer.HTTPStart();

}

void HomeKit::Start() {
	if (TaskHandle == NULL)
		TaskHandle = FreeRTOS::StartTask(Task, "hap_bridge", NULL, 4096, 6);
}


void HomeKit::Stop() {
	if (IsRunning)
		::hap_stop();
}

void HomeKit::AppServerRestart() {
	Stop();
	FillAccessories();
	Start();
	return;
}

void HomeKit::ResetPairs() {
	::hap_reset_pairings();
}

void HomeKit::ResetData() {
	::hap_reset_homekit_data();
}

bool HomeKit::IsEnabledForDevice() {
	return (Mode != ModeEnum::NONE);
}

bool HomeKit::IsExperimentalMode() {
	return (Mode == ModeEnum::EXPERIMENTAL);
}



/* Mandatory identify routine for the accessory (bridge)
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
int HomeKit::BridgeIdentify(hap_acc_t *ha) {
    ESP_LOGI(Tag, "Bridge identified");
    Log::Add(Log::Events::Misc::HomeKitIdentify);
    return HAP_SUCCESS;
}

/* Mandatory identify routine for the bridged accessory
 * In a real bridge, the actual accessory must be sent some request to
 * identify itself visually
 */
int HomeKit::AccessoryIdentify(hap_acc_t *ha)
{
    ESP_LOGI(Tag, "Bridge identified");
    Log::Add(Log::Events::Misc::HomeKitIdentify);
    return HAP_SUCCESS;
}

hap_status_t HomeKit::On(bool Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	if (Iterator > 0) return HAP_STATUS_SUCCESS;

    	ESP_LOGE("ON", "UUID: %04X, Value: %d, Iterator: %d", AID, Value, Iterator);

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.DeviceType == 0x01 && (uint32_t)(Time::UptimeU() - TVHIDLastUpdated) < 500000)
        	return HAP_STATUS_SUCCESS;

        if (IRDeviceItem.IsEmpty())
        	return HAP_STATUS_VAL_INVALID;

        if (IRDeviceItem.DeviceType == 0xEF) { // air conditioner
        	// check service. If fan for ac - skip
        	if (strcmp(hap_serv_get_type_uuid(hap_char_get_parent(Char)), HAP_SERV_UUID_FAN_V2) == 0)
        		return HAP_STATUS_SUCCESS;

        	uint8_t NewValue = 0;
        	uint8_t CurrentMode = ((DataRemote_t*)Data)->DevicesHelper.GetDeviceForType(0xEF)->GetStatusByte(IRDeviceItem.Status , 0);

        	//on

        	if (Value == 0) {
        		hap_val_t ValueForACFanActive;
        		ValueForACFanActive.u = 0;
        		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ACTIVE, ValueForACFanActive);

        		hap_val_t ValueForACFanState;
        		ValueForACFanState.f = 0;
        		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ROTATION_SPEED, ValueForACFanState);

        		hap_val_t ValueForACFanAuto;
        		ValueForACFanAuto.u = 0;
        		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_TARGET_FAN_STATE, ValueForACFanAuto);

        		StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, 0);
        	}
        	else
        	{
        		if (IRDeviceItem.Status < 0x1000) {
                    CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

                    if (IRCommand != nullptr) {
                    	string Operand = Converter::ToHexString(IRDeviceItem.Extra, 4) + "FFF0";
                    	IRCommand->Execute(0xEF, Operand.c_str());
                        FreeRTOS::Sleep(1000);
                    }
        		}
        		else
        			return HAP_STATUS_SUCCESS;

        		/*
                if (IRDeviceItem.Status < 0x1000) && ACOperand::IsOnSeparateForCodeset(IRDeviceItem.Extra))
                {
                	ESP_LOGE("HomeKit", "Switch ON AC");

                    CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

                    if (IRCommand != nullptr) {
                    	string Operand = Converter::ToHexString(IRDeviceItem.Extra, 4) + "FFF0";
                        IRCommand->Execute(0xEF, Operand.c_str());
                        FreeRTOS::Sleep(1000);
                    }
                }
                */
        	}

        	return HAP_STATUS_SUCCESS;
        }

        ((DataRemote_t*)Data)->StatusUpdateForDevice(IRDeviceItem.DeviceID, 0xE0, Value);
        map<string,string> Functions = ((DataRemote_t*)Data)->LoadDeviceFunctions(IRDeviceItem.DeviceID);

        CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

        string Operand = IRDeviceItem.DeviceID;

        if (Functions.count("power") > 0)
        {
        	Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("power"),2);
        	Operand += (Functions["power"] == "toggle") ? ((Value) ? "00" : "01" ) : "FF";
        	IRCommand->Execute(0xFE, Operand.c_str());

        	return HAP_STATUS_SUCCESS;
        }
        else
        {
        	if (Functions.count("poweron") > 0 && Value)
        	{
            	Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("poweron"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
            	return HAP_STATUS_SUCCESS;
        	}

        	if (Functions.count("poweroff") > 0 && !Value)
        	{
            	Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("poweroff"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
            	return HAP_STATUS_SUCCESS;
        	}
        }
    }

    return HAP_STATUS_VAL_INVALID;
}

bool HomeKit::Cursor(uint8_t Value, uint16_t AccessoryID) {
	string UUID = Converter::ToHexString(AccessoryID, 4);
	ESP_LOGE("Cursor for UUID", "%s Value: %d", UUID.c_str(), Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	TVHIDLastUpdated = Time::UptimeU();

        map<string,string> Functions = ((DataRemote_t*)Data)->LoadDeviceFunctions(UUID);

        CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

        string Operand = UUID;

        if (Value == 8 || Value == 6 || Value == 4 || Value == 7 || Value == 5) {
        	if (Functions.count("cursor") > 0)
        	{
        		Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("cursor"),2);

        		switch(Value) {
    				case 8: Operand += "00"; break; // SELECT
    				case 6: Operand += "01"; break; // LEFT
    				case 4: Operand += "02"; break; // UP
    				case 7: Operand += "03"; break; // RIGTH
    				case 5: Operand += "04"; break; // DOWN
    				default:
    					return false;
        		}

        		IRCommand->Execute(0xFE, Operand.c_str());
        		return true;
        	}
        }

        if (Value == 15) {
        	if (Functions.count("menu") > 0)
        	{
        		Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("menu"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
        		return true;
        	}
        }

        if (Value == 11) {
        	if (Functions.count("play") > 0)
        	{
        		Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("play"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
        		return true;
        	}
        }

        if (Value == 9) {
        	if (Functions.count("back") > 0)
        	{
        		Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("back"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
        		return true;
        	}
        }
    }

    return false;
}

bool HomeKit::ActiveID(uint8_t NewActiveID, uint16_t AccessoryID) {
	string UUID = Converter::ToHexString(AccessoryID, 4);
	ESP_LOGE("ActiveID for UUID", "%s", UUID.c_str());

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        map<string,string> Functions = ((DataRemote_t*)Data)->LoadDeviceFunctions(UUID);

        CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

        if (Functions.count("mode") > 0)
        {
            string Operand = UUID;
        	Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("mode"),2);
        	Operand += Converter::ToHexString(NewActiveID - 1, 2);

        	IRCommand->Execute(0xFE, Operand.c_str());
        	return true;
        }
    }

    return false;
}

bool HomeKit::Volume(uint8_t Value, uint16_t AccessoryID) {
	string UUID = Converter::ToHexString(AccessoryID, 4);
	ESP_LOGE("Volume", "UUID: %s, Value, %d", UUID.c_str(), Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	TVHIDLastUpdated = Time::UptimeU();

        map<string,string> Functions = ((DataRemote_t*)Data)->LoadDeviceFunctions(UUID);

        CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

        if (Value == 0 && Functions.count("volup") > 0)
        {
            string Operand = UUID + Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("volup"),2) + "FF";
        	IRCommand->Execute(0xFE, Operand.c_str());
        	IRCommand->Execute(0xED, "");
        	IRCommand->Execute(0xED, "");

        	return true;
        }
        else if (Value == 1 && Functions.count("voldown") > 0)
        {
            string Operand = UUID + Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("voldown"),2) + "FF";
        	IRCommand->Execute(0xFE, Operand.c_str());
        	IRCommand->Execute(0xED, "");
        	IRCommand->Execute(0xED, "");
        	return true;
        }
    }

    return false;
}

bool HomeKit::HeaterCoolerState(uint8_t Value, uint16_t AID) {
	ESP_LOGE("HeaterCoolerState", "UUID: %04X, Value: %d", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
        	return false;

        switch (Value)
        {
        	case 0: Value = 1; break;
        	case 1:	Value = 3; break;
        	case 2: Value = 2; break;
        	default: break;
        }

        // change temperature because of it may depends on selected mode
        const hap_val_t* NewTempValue  = HomeKitGetCharValue(AID, HAP_SERV_UUID_HEATER_COOLER,
        		(Value == 3) ? HAP_CHAR_UUID_HEATING_THRESHOLD_TEMPERATURE : HAP_CHAR_UUID_COOLING_THRESHOLD_TEMPERATURE);

        if (NewTempValue != NULL)
            StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra, 0xE1, round(NewTempValue->f), false);

    	if (Value > 0) {
            // change current heater cooler state
            hap_val_t CurrentHeaterCoolerState;


            CurrentHeaterCoolerState.u = 0;
            if (Value == 1) CurrentHeaterCoolerState.u = 2;
            if (Value == 2) CurrentHeaterCoolerState.u = 3;

            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_HEATER_COOLER_STATE, CurrentHeaterCoolerState);

    		hap_val_t ValueForACFanActive;
    		ValueForACFanActive.u = 1;
    		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ACTIVE, ValueForACFanActive);

    		hap_val_t ValueForACFanState;
    		hap_val_t ValueForACFanAuto;

    		uint8_t FanStatus = DataDeviceItem_t::GetStatusByte(IRDeviceItem.Status, 2);

    		ValueForACFanState.f 	= (FanStatus > 0)	? FanStatus : 2;
    		ValueForACFanAuto.u 	= (FanStatus == 0) 	? 1 : 0;

    		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ROTATION_SPEED, ValueForACFanState);
    		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_TARGET_FAN_STATE, ValueForACFanAuto);
    	}


        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, Value);

        return true;
    }

    return false;
}

bool HomeKit::ThresholdTemperature(float Value, uint16_t AID, bool IsCooling) {
	ESP_LOGE("TargetTemperature", "UUID: %04X, Value: %f", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
        	return false;

        if (Value > 30) Value = 30;
        if (Value < 16) Value = 16;

        hap_val_t HAPValue;
        HAPValue.f = Value;

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE1, round(Value));
        return true;
    }

    return false;
}

bool HomeKit::RotationSpeed(float Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {
	ESP_LOGE("RotationSpeed", "UUID: %04X, Value: %f", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

    	if (Iterator > 0 && IRDeviceItem.DeviceType != 0xEF) return true;

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // Air Conditionair
        	return false;

        if ((Value > 0 && IRDeviceItem.Status < 0x1000) || (Value == 0 && IRDeviceItem.Status >= 0x1000)) {
            hap_val_t ValueForACActive;
            ValueForACActive.u = (Value == 0) ? 0 : 1;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_ACTIVE, ValueForACActive);

            hap_val_t ValueForACState;
            ValueForACState.u = (Value == 0) ? 0 : 3;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_HEATER_COOLER_STATE, ValueForACState);

            StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, (Value == 0) ? 0 : 2, false);

            hap_val_t ValueForACFanAuto;
            ValueForACFanAuto.u = 0;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_TARGET_FAN_STATE, ValueForACFanAuto);
    	}

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE2, round(Value));

        return true;
    }

    return false;
}

bool HomeKit::TargetFanState(bool Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {
	ESP_LOGE("TargetFanState", "UUID: %04X, Value: %d, Iterator: %d", AID, Value, Iterator);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	if (Iterator > 0) return true;

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditioner
        	return false;

        // switch on
        if (Value > 0 && IRDeviceItem.Status < 0x1000)
        {
        	hap_val_t ValueForACFanActive;
        	ValueForACFanActive.u = 1;
        	HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ACTIVE, ValueForACFanActive);

        	hap_val_t ValueForACFanState;

        	uint8_t FanStatus = DataDeviceItem_t::GetStatusByte(IRDeviceItem.Status, 2);
        	ValueForACFanState.f 	= (FanStatus > 0)	? FanStatus : 2;

        	HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ROTATION_SPEED, ValueForACFanState);

//			hap_val_t ValueForACFanAuto;
//        	ValueForACFanAuto.u 	= (FanStatus == 0) 	? 1 : 0;
//        	HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_TARGET_FAN_STATE, ValueForACFanAuto);

            hap_val_t ValueForACActive;
            ValueForACActive.u = 1;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_ACTIVE, ValueForACActive);

            hap_val_t ValueForACState;
            ValueForACState.u = 3;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_HEATER_COOLER_STATE, ValueForACState);

            StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, 2, false);
        }

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE2, (Value) ? 0 : 2);

        return true;
    }

    return false;
}

bool HomeKit::SwingMode(bool Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {
	ESP_LOGE("TargetFanState", "UUID: %04X, Value: %d", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
        	return false;

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE3, (Value) ? 1 : 0);

        return true;
    }

    return false;
}

bool HomeKit::GetConfiguredName(char* Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {


	ESP_LOGE("GetConfiguredName", "UUID: %04X, Value: %s", AID, Value);
	uint32_t IID = hap_serv_get_iid(hap_char_get_parent(Char));


	/*
	string KeyName = string(NVS_HOMEKIT_CNPREFIX) + Converter::ToHexString(AID, 4) + Converter::ToHexString(IID, 8);

	ESP_LOGE("!", "%s", KeyName.c_str());

	NVS Memory(NVS_HOMEKIT_AREA);
	Memory.SetString(Converter::ToLower(KeyName), string(Value));
	Memory.Commit();
	 */
	return true;
}

bool HomeKit::SetConfiguredName(char* Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {
	ESP_LOGE("SetConfiguredName", "UUID: %04X, Value: %s", AID, Value);
	uint32_t IID = hap_serv_get_iid(hap_char_get_parent(Char));

	string KeyName = string(NVS_HOMEKIT_CNPREFIX) + Converter::ToHexString(AID, 4) + Converter::ToHexString(IID, 8);

	ESP_LOGE("!", "%s", KeyName.c_str());

	NVS Memory(NVS_HOMEKIT_AREA);
	Memory.SetString(Converter::ToLower(KeyName), string(Value));
	Memory.Commit();

	return true;
}


void HomeKit::StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value, bool Send) {
	pair<bool, uint16_t> Result = ((DataRemote_t*)Data)->StatusUpdateForDevice(UUID, FunctionID, Value, "", false);

	ESP_LOGE("RESULT", "StatusACUpdateIRSend %02X %02X %u Result.second %04X", FunctionID, Value, Send, Result.second);

	if (!Send) return;

	if (!Result.first) return;
	if (Codeset == 0) return;

	/*
	// Если было ноль а стало не ноль - отправить FFF0
	// Если было не ноль, а стало ноль - отправить как есть
	if (FunctionID == 0xE0) {
		uint8_t OldMode = (uint8_t)(Result.second >> 12);
		uint8_t NewMode	= (uint8_t)(Value >> 12);

		if (OldMode == 0 && NewMode > 0)
			Result.second = 0xFFF0;

		ESP_LOGE("StatusACUpdateIRSend", "OldMode %u NewMode %u", OldMode, NewMode);
	}
	 */

    CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

    if (IRCommand == nullptr) return;

    string Operand = Converter::ToHexString(Codeset,4);

    if (FunctionID == 0xE3 && ACOperand::IsSwingSeparateForCodeset(Codeset))
    	Operand += ("FFF" + Converter::ToHexString((Value == 0) ? 2 : 1,1));
    else
    	Operand += Converter::ToHexString(Result.second,4);

    IRCommand->Execute(0xEF, Operand.c_str());
}



/* A dummy callback for handling a write on the "On" characteristic of Fan.
 * In an actual accessory, this should control the hardware
 */
int HomeKit::WriteCallback(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv)
{
    int i, ret = HAP_SUCCESS;

    uint16_t AID = (uint16_t)((uint32_t)(serv_priv));
    ESP_LOGE(Tag, "Write called for Accessory with AID %04X", AID);

    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];

        ESP_LOGI(Tag, "Characteristic: %s, i: %d", hap_char_get_type_uuid(write->hc), i);

        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
        	*(write->status) = On(write->val.b, AID, write->hc, i);
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ACTIVE)) {
        	*(write->status) = On(write->val.b, AID, write->hc, i);

        	if (Settings.eFuse.Type == Settings.Devices.Remote) {
                DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

            	if (*(write->status) == HAP_STATUS_SUCCESS) {
        			static hap_val_t HAPValue;

            		if (IRDeviceItem.DeviceType == 0x01) {
            			HAPValue.u = (uint8_t)write->val.b;
            			HomeKitUpdateCharValue(AID, SERVICE_TV_UUID, CHAR_SLEEP_DISCOVERY_UUID, HAPValue);
            		}

                   	if (IRDeviceItem.DeviceType == 0x05) {
              //              CurrentPurifierActive.u = (Value > 0) ? 1 : 0;
                //    		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_AIR_PURIFIER, HAP_CHAR_UUID_ACTIVE, CurrentPurifierActive);
                   		HAPValue.u = ((uint8_t)write->val.b == 1) ? 2 : 0;
                   		HomeKitUpdateCharValue(AID, HAP_SERV_UUID_AIR_PURIFIER, HAP_CHAR_UUID_CURRENT_AIR_PURIFIER_STATE, HAPValue);
                    }
            	}
        	}
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_REMOTEKEY_UUID)) {
        	Cursor(write->val.b, AID);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_ACTIVE_IDENTIFIER_UUID)) {
        	ActiveID(write->val.b, AID);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_VOLUME_SELECTOR_UUID)) {
        	Volume(write->val.b, AID);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_HEATER_COOLER_STATE)) {
        	HeaterCoolerState(write->val.b, AID);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_COOLING_THRESHOLD_TEMPERATURE)) {
        	ThresholdTemperature(write->val.f, AID, true);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_HEATING_THRESHOLD_TEMPERATURE)) {
        	ThresholdTemperature(write->val.f, AID, false);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ROTATION_SPEED)) {
        	RotationSpeed(write->val.f, AID, write->hc, i);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_FAN_STATE)) {
        	TargetFanState(write->val.b, AID, write->hc, i);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SWING_MODE)) {
        	SwingMode(write->val.b, AID, write->hc, i);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_CONFIGUREDNAME_UUID)) {
        	//ConfiguredName(write->val.s, AID, write->hc, i);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_IS_CONFIGURED)) {
        	*(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_AIR_PURIFIER_STATE)) {
        	*(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_MUTE_UUID)) {
        	*(write->status) = HAP_STATUS_SUCCESS;

        	if (Settings.eFuse.Type == Settings.Devices.Remote) {
                DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

            	if (*(write->status) == HAP_STATUS_SUCCESS && IRDeviceItem.DeviceType == 0x01) {
        			static hap_val_t HAPValue;
        			HAPValue.u = (write->val.b == true) ? 0 : 1;
        			HomeKitUpdateCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_VOLUME_CONTROL_TYPE_UUID, HAPValue);
            	}
        	}
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_VOLUME_UUID)) {
        	*(write->status) = HAP_STATUS_SUCCESS;

        	if (Settings.eFuse.Type == Settings.Devices.Remote) {
                DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

                if (*(write->status) == HAP_STATUS_SUCCESS && IRDeviceItem.DeviceType == 0x01) {
                	static hap_val_t HAPValueMute;
                	HAPValueMute.b = false;
                	HomeKitUpdateCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_MUTE_UUID, HAPValueMute);

        			static hap_val_t HAPValueControlType;
        			HAPValueControlType.u = 1;
        			HomeKitUpdateCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_VOLUME_CONTROL_TYPE_UUID, HAPValueControlType);
                }
        	}
        }
        else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
        }

        if (*(write->status) == HAP_STATUS_SUCCESS)
        	hap_char_update_val(write->hc, &(write->val));
        else
            ret = HAP_FAIL;
    }

    return ret;
}

hap_cid_t HomeKit::FillAccessories() {
	hap_acc_t 	*Accessory = NULL;

	hap_set_debug_level(HAP_DEBUG_LEVEL_WARN);

	switch (Settings.eFuse.Type) {
		case 0x81:
			return (Mode == ModeEnum::BASIC) ? FillRemoteACOnly(Accessory) : FillRemoteBridge(Accessory);
			break;
	}

	return HAP_CID_NONE;
}


hap_cid_t HomeKit::FillRemoteACOnly(hap_acc_t *Accessory) {
	string 		NameString 	=  "";
	uint16_t 	UUID 		= 0;
	uint16_t	Status		= 0;

	for (auto &IRCachedDevice : ((DataRemote_t *)Data)->IRDevicesCache)
		if (IRCachedDevice.DeviceType == 0xEF) {
			DataRemote_t::IRDevice LoadedDevice = ((DataRemote_t *)Data)->GetDevice(IRCachedDevice.DeviceID);

			NameString 	= LoadedDevice.Name;
			UUID 		= Converter::UintFromHexString<uint16_t>(LoadedDevice.UUID);
			Status		= LoadedDevice.Status;
			break;
		}

	hap_acc_t* ExistedAccessory = hap_get_first_acc();

	if (ExistedAccessory == NULL)
	{
		NameString = Converter::CutMultibyteString(NameString, 16);

		if (NameString == "")
			NameString = Device.TypeToString() + " " + Device.IDToString();

		static AccessoryData_t AccessoryData(NameString, Device.ModelToString(), Device.IDToString());

		hap_acc_cfg_t cfg = {
			.name 				= AccessoryData.Name,
			.model 				= AccessoryData.Model,
			.manufacturer 		= "LOOKin",
			.serial_num 		= AccessoryData.ID,
			.fw_rev 			= strdup(Settings.Firmware.ToString().c_str()),
			.hw_rev 			= NULL,
			.pv 				= "1.1.0",
			.cid 				= HAP_CID_AIR_CONDITIONER,
			.identify_routine 	= BridgeIdentify,
		};

		Accessory = hap_acc_create(&cfg);

		ACOperand Operand((uint32_t)Status);

		hap_serv_t *ServiceAC;
		ServiceAC = hap_serv_ac_create((Operand.Mode==0) ? 0 : 3, Operand.Temperature, Operand.Temperature, 0);
		hap_serv_add_char(ServiceAC, hap_char_name_create("Air Conditioner"));
		hap_serv_set_priv(ServiceAC, (void *)(uint32_t)UUID);
		hap_serv_set_write_cb(ServiceAC, WriteCallback);
		hap_acc_add_serv(Accessory, ServiceAC);

		hap_serv_t *ServiceACFan;
		ServiceACFan = hap_serv_ac_fan_create((Operand.Mode == 0) ? false : true, (Operand.FanMode == 0) ? 1 : 0, Operand.SwingMode, Operand.FanMode);
		hap_serv_add_char(ServiceACFan, hap_char_name_create("Swing & Ventillation"));
		hap_serv_set_priv(ServiceACFan, (void *)(uint32_t)UUID);
		hap_serv_set_write_cb(ServiceACFan, WriteCallback);
		//hap_serv_link_serv(ServiceAC, ServiceACFan);
		hap_acc_add_serv(Accessory, ServiceACFan);

		float HumidityValue = 0;
		Sensor_t* SensorMeteo = Sensor_t::GetSensorByID(0xFE);
		if (SensorMeteo != nullptr)
			HumidityValue = Converter::ToFloat(SensorMeteo->FormatValue("Humidity"));

		hap_serv_t *ServiceACHumidity;
		ServiceACHumidity = hap_serv_humidity_sensor_create(HumidityValue);
		hap_serv_add_char(ServiceACHumidity, hap_char_name_create("Humidity"));
		hap_serv_set_priv(ServiceACHumidity, (void *)(uint32_t)UUID);
		hap_serv_set_write_cb(ServiceACHumidity, WriteCallback);
		//hap_serv_link_serv(ServiceAC, ServiceACFan);
		hap_acc_add_serv(Accessory, ServiceACHumidity);


		uint8_t product_data[] = {0x4D, 0x7E, 0xC5, 0x46, 0x80, 0x79, 0x26, 0x54};
		hap_acc_add_product_data(Accessory, product_data, sizeof(product_data));

		hap_add_accessory(Accessory);
	}
	else
	{
		hap_serv_t *ACService = hap_acc_get_serv_by_uuid(ExistedAccessory, HAP_SERV_UUID_HEATER_COOLER);
		if (ACService != NULL)
			hap_serv_set_priv(ACService, (void *)(uint32_t)UUID);

		hap_serv_t *ACFanService = hap_acc_get_serv_by_uuid(ExistedAccessory, HAP_SERV_UUID_FAN_V2);
		if (ACFanService != NULL)
			hap_serv_set_priv(ACFanService, (void *)(uint32_t)UUID);
	}

	return HAP_CID_AIR_CONDITIONER;
}

hap_cid_t HomeKit::FillRemoteBridge(hap_acc_t *Accessory) {
	if (BridgedAccessories.size() == 0) {
		string BridgeNameString = Device.GetName();

		BridgeNameString = Converter::CutMultibyteString(BridgeNameString, 16);
		if (BridgeNameString == "") BridgeNameString = Device.TypeToString() + " " + Device.IDToString();

		static AccessoryData_t AccessoryData(BridgeNameString, Device.ModelToString(), Device.IDToString());

		hap_acc_cfg_t cfg = {
			.name 				= AccessoryData.Name,
			.model 				= AccessoryData.Model,
			.manufacturer 		= "LOOKin",
			.serial_num 		= AccessoryData.ID,
			.fw_rev 			= strdup(Settings.Firmware.ToString().c_str()),
			.hw_rev 			= NULL,
			.pv 				= "1.1.0",
			.cid 				= HAP_CID_BRIDGE,
			.identify_routine 	= BridgeIdentify,
		};

		Accessory = hap_acc_create(&cfg);

		uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
		hap_acc_add_product_data(Accessory, product_data, sizeof(product_data));

		hap_add_accessory(Accessory);
	}

	for(auto& BridgeAccessory : BridgedAccessories) {
		hap_remove_bridged_accessory(BridgeAccessory);
		hap_acc_delete(BridgeAccessory);
	}

	BridgedAccessories.clear();

	for (auto &IRCachedDevice : ((DataRemote_t *)Data)->IRDevicesCache)
	{
		DataRemote_t::IRDevice IRDevice = ((DataRemote_t *)Data)->GetDevice(IRCachedDevice.DeviceID);

		char accessory_name[16] = {0};
		string Name = Converter::CutMultibyteString(IRDevice.Name, 16);

		sprintf(accessory_name, "%s", (Name != "") ? Name.c_str() : "Accessory\0");

		hap_acc_cfg_t bridge_cfg = {
			.name 				= accessory_name,
			.model 				= "n/a",
			.manufacturer 		= "n/a",
			.serial_num 		= "n/a",
			.fw_rev 			= strdup(Settings.Firmware.ToString().c_str()),
            .hw_rev 			= NULL,
			.pv 				= "1.1.0",
			.cid 				= HAP_CID_BRIDGE,
			.identify_routine 	= AccessoryIdentify,
		};

		uint16_t AID = Converter::UintFromHexString<uint16_t>(IRDevice.UUID);

		Accessory = hap_acc_create(&bridge_cfg);

		bool IsCreated = true;

		switch (IRDevice.Type) {
			case 0x01: // TV
			{
				hap_serv_t *ServiceTV;

				uint8_t PowerStatus = DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);
				uint8_t ModeStatus 	= DataDeviceItem_t::GetStatusByte(IRDevice.Status, 1);

				if (PowerStatus > 1) PowerStatus = 1;

				/*
				if (ModeStatus >= IRDevice.Functions.count("mode"))
					ModeStatus = 0;
				*/

				ServiceTV = hap_serv_tv_create(PowerStatus, ModeStatus + 1, &accessory_name[0]);
				hap_serv_add_char		(ServiceTV, hap_char_name_create(accessory_name));

				hap_serv_set_priv		(ServiceTV, (void *)(uint32_t)AID);
				hap_serv_set_write_cb	(ServiceTV, WriteCallback);
				//hap_serv_set_read_cb	(ServiceTC, ReadCallback)
				hap_acc_add_serv		(Accessory, ServiceTV);

				if (IRDevice.Functions.count("volup") > 0 || IRDevice.Functions.count("voldown") > 0) {
					hap_serv_t *ServiceSpeaker;
					ServiceSpeaker = hap_serv_tv_speaker_create(false);
					hap_serv_add_char		(ServiceSpeaker, hap_char_name_create(accessory_name));

					hap_serv_set_priv		(ServiceSpeaker, (void *)(uint32_t)AID);
					hap_serv_set_write_cb	(ServiceSpeaker, WriteCallback);
					//hap_serv_set_read_cb	(ServiceTC, ReadCallback);

					hap_serv_link_serv		(ServiceTV, ServiceSpeaker);
					hap_acc_add_serv		(Accessory, ServiceSpeaker);
				}

				for (uint8_t i = 0; i < ((DataRemote_t *)Data)->LoadAllFunctionSignals(IRDevice.UUID, "mode", IRDevice).size(); i++)
				{
					char InputSourceName[17];
				    sprintf(InputSourceName, "Input source %d", i+1);

					hap_serv_t *ServiceInput;
					ServiceInput = hap_serv_input_source_create(&InputSourceName[0], i+1);

					hap_serv_set_priv(ServiceInput, (void *)(uint32_t)AID);
					hap_serv_set_write_cb(ServiceInput, WriteCallback);
					//hap_serv_set_read_cb	(ServiceTC, ReadCallback)

					hap_serv_link_serv(ServiceTV, ServiceInput);
					hap_acc_add_serv(Accessory, ServiceInput);
				}

				break;
			}
			case 0x03: // light
				hap_serv_t *ServiceLight;
				ServiceLight = hap_serv_lightbulb_create(false);
				hap_serv_add_char(ServiceLight, hap_char_name_create(accessory_name));

				hap_serv_set_priv(ServiceLight, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServiceLight, WriteCallback);
				hap_acc_add_serv(Accessory, ServiceLight);

				break;
			case 0x05: { // Purifier
				uint8_t PowerStatus 	= DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);
				if (PowerStatus > 0) PowerStatus = 1;

				hap_serv_t *ServicePurifier;
				ServicePurifier = hap_serv_air_purifier_create(PowerStatus, (PowerStatus > 0) ? 2 : 0 , 0);
				hap_serv_add_char(ServicePurifier, hap_char_name_create(accessory_name));

				hap_serv_set_priv(ServicePurifier, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServicePurifier, WriteCallback);
				hap_acc_add_serv(Accessory, ServicePurifier);
				break;
			}

			case 0x06: // Vacuum cleaner
			{
				hap_serv_t *ServiceVacuumCleaner;

				uint8_t PowerStatus = DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);

				ServiceVacuumCleaner = hap_serv_switch_create((bool)(PowerStatus > 0));
				hap_serv_add_char(ServiceVacuumCleaner, hap_char_name_create(accessory_name));

				hap_serv_set_priv(ServiceVacuumCleaner, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServiceVacuumCleaner, WriteCallback);
				hap_acc_add_serv(Accessory, ServiceVacuumCleaner);
				break;
			}
			case 0x07: // Fan
			{
				uint8_t PowerStatus 	= DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);

				hap_serv_t *ServiceFan;
				ServiceFan = hap_serv_fan_v2_create(PowerStatus);
				hap_serv_add_char(ServiceFan, hap_char_name_create(accessory_name));

				hap_serv_set_priv(ServiceFan, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServiceFan, WriteCallback);
				hap_acc_add_serv(Accessory, ServiceFan);
				break;
			}
			case 0xEF: { // AC
				ACOperand Operand((uint32_t)IRDevice.Status);

				hap_serv_t *ServiceAC;
				ServiceAC = hap_serv_ac_create((Operand.Mode==0) ? 0 : 3, Operand.Temperature, Operand.Temperature, 0);
				hap_serv_add_char(ServiceAC, hap_char_name_create("Air Conditioner"));
				hap_serv_set_priv(ServiceAC, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServiceAC, WriteCallback);
				hap_acc_add_serv(Accessory, ServiceAC);

				hap_serv_t *ServiceACFan;
				ServiceACFan = hap_serv_ac_fan_create((Operand.Mode == 0) ? false : true, (Operand.FanMode == 0) ? 1 : 0, Operand.SwingMode, Operand.FanMode);
				hap_serv_add_char(ServiceACFan, hap_char_name_create("Swing & Ventillation"));

				hap_serv_set_priv(ServiceACFan, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServiceACFan, WriteCallback);
				//hap_serv_link_serv(ServiceAC, ServiceACFan);
				hap_acc_add_serv(Accessory, ServiceACFan);
				break;
			}
			default:
				IsCreated = false;
				hap_acc_delete(Accessory);
			}

		if (IsCreated) {
			BridgedAccessories.push_back(Accessory);
			hap_add_bridged_accessory(Accessory, AID);
		}
	}

	SensorMeteo_t *Meteo = (SensorMeteo_t *)Sensor_t::GetSensorByID(0xFE);

	if (Meteo != nullptr && Meteo->GetSIEFlag()) {
		hap_acc_cfg_t hsensor_cfg = {
			.name 				= "Humidity Sensor",
			.model 				= "n/a",
			.manufacturer 		= "LOOKin",
			.serial_num 		= "n/a",
			.fw_rev 			= strdup(Settings.Firmware.ToString().c_str()),
            .hw_rev 			= NULL,
			.pv 				= "1.1.0",
			.cid 				= HAP_CID_BRIDGE,
			.identify_routine 	= AccessoryIdentify,
		};

		Accessory = hap_acc_create(&hsensor_cfg);

		hap_serv_t *HumiditySensor = hap_serv_humidity_sensor_create(Meteo->ConvertToFloat(Meteo->ReceiveValue("Humidity")));
		hap_acc_add_serv(Accessory, HumiditySensor);
		hap_add_bridged_accessory(Accessory, 0xFFFF);

		hap_acc_cfg_t tsensor_cfg = {
			.name 				= "Temp sensor",
			.model 				= "n/a",
			.manufacturer 		= "LOOKin",
			.serial_num 		= "n/a",
			.fw_rev 			= strdup(Settings.Firmware.ToString().c_str()),
            .hw_rev 			= NULL,
			.pv 				= "1.1.0",
			.cid 				= HAP_CID_BRIDGE,
			.identify_routine 	= AccessoryIdentify,
		};

		Accessory = hap_acc_create(&tsensor_cfg);

		hap_serv_t *TemperatureSensor = hap_serv_temperature_sensor_create(Meteo->ConvertToFloat(Meteo->ReceiveValue("Temperature")));
		hap_acc_add_serv(Accessory, TemperatureSensor);

		hap_add_bridged_accessory(Accessory, 0xFFFE);
	}

	return HAP_CID_BRIDGE;
}

void HomeKit::Task(void *) {
	IsRunning = true;

	hap_set_debug_level(HAP_DEBUG_LEVEL_INFO);

    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_NAME;

    hap_set_config(&hap_cfg);

	/* Initialize the HAP core */
	hap_init(HAP_TRANSPORT_WIFI);

	hap_cid_t CID = FillAccessories();

    /* Enable WAC2 as per HAP Spec R12 */
    //hap_enable_wac2();

	//ESP_LOGI(Tag, "Use setup payload: \"X-HM://00LCC17KE1234\" for Accessory Setup");

	if (Mode == ModeEnum::BASIC)
	{
		string 	Pin 	= "";
		string	SetupID = "";

		string Partition = "factory_nvs";
		NVS::Init(Partition);
		NVS MemoryHAP(Partition, "hap_custom");

		SetupID = MemoryHAP.GetString("setupid");
		Pin 	= MemoryHAP.GetString("pin");

		NVS::Deinit(Partition);

		if (Settings.eFuse.DeviceID == 0x98F33256 || Settings.eFuse.DeviceID == 0x98F33257 || Settings.eFuse.DeviceID == 0x98F33258 || Settings.eFuse.DeviceID == 0x98F3328F)
		{
			SetupID = "";
			Pin = "";
		}

		if (SetupID == "" || Pin == "") {
			switch (Settings.eFuse.DeviceID)  {
				case 0x00000003: case 0x98F33002: case 0x98F33008: case 0x98F3300E: case 0x98F33014: case 0x98F3301A: case 0x98F33020: case 0x98F33026: case 0x98F3303B: case 0x98F33041:
					Pin 	= "354-86-394";
					SetupID = "QP07";
					break;

				case 0x98F33001: case 0x98F33007: case 0x98F3300D: case 0x98F33013: case 0x98F33019: case 0x98F3301F: case 0x98F33025: case 0x98F3303A: case 0x98F33040:
					Pin 	= "483-27-621";
					SetupID = "ER11";
					break;

				case 0x98F33005: case 0x98F3300B: case 0x98F33011: case 0x98F33017: case 0x98F3301D: case 0x98F33023: case 0x98F33029: case 0x98F3303E:
					Pin 	= "192-01-049";
					SetupID = "POMJ";
					break;

				case 0x98F33006: case 0x98F3300C: case 0x98F33012: case 0x98F33018: case 0x98F3301E: case 0x98F33024: case 0x98F33030: case 0x98F3303F:
					Pin 	= "548-68-670";
					SetupID = "DKJU";
					break;

				case 0x98F33004: case 0x98F3300A: case 0x98F33010: case 0x98F33016: case 0x98F3301C: case 0x98F33022: case 0x98F33028: case 0x98F3303D:
					Pin 	= "729-17-934";
					SetupID = "DCIU";
					break;

				case 0x98F33003: case 0x98F33009: case 0x98F3300F: case 0x98F33015: case 0x98F3301B: case 0x98F33021: case 0x98F33027: case 0x98F3303C:
					Pin 	= "905-86-070";
					SetupID = "FF11";
					break;

				case 0x00000004: Pin = "123-45-678"; SetupID = "ABCD"; break;
				case 0x98F3302A: Pin = "488-85-380"; SetupID = "VXAE"; break;
				case 0x98F3302B: Pin = "918-21-683"; SetupID = "06WA"; break;
				case 0x98F3302C: Pin = "311-01-079"; SetupID = "E049"; break;
				case 0x98F3302D: Pin = "476-74-974"; SetupID = "GGYQ"; break;
				case 0x98F3302E: Pin = "558-44-407"; SetupID = "3KH4"; break;
				case 0x98F3302F: Pin = "912-93-933"; SetupID = "HMYV"; break;
				case 0x98F33031: Pin = "386-10-943"; SetupID = "E2YF"; break;
				case 0x98F33032: Pin = "031-86-050"; SetupID = "PPC2"; break;
				case 0x98F33033: Pin = "766-00-867"; SetupID = "IP50"; break;
				case 0x98F33034: Pin = "871-29-689"; SetupID = "8AIG"; break;
				case 0x98F33035: Pin = "963-02-487"; SetupID = "CZGJ"; break;
				case 0x98F33036: Pin = "210-30-134"; SetupID = "O3KK"; break;
				case 0x98F33037: Pin = "576-32-541"; SetupID = "H039"; break;
				case 0x98F33038: Pin = "140-97-060"; SetupID = "16RI"; break;
				case 0x98F33039: Pin = "611-76-935"; SetupID = "9M43"; break;
				case 0x98F33042: Pin = "835-80-299"; SetupID = "EUNL"; break;
				case 0x98F33043: Pin = "487-92-510"; SetupID = "MZ3O"; break;
				case 0x98F33044: Pin = "663-58-001"; SetupID = "OFNT"; break;
				case 0x98F33045: Pin = "520-36-031"; SetupID = "TC4G"; break;
				case 0x98F33046: Pin = "532-02-426"; SetupID = "4ITH"; break;
				case 0x98F33047: Pin = "403-35-631"; SetupID = "BD5B"; break;
				case 0x98F33048: Pin = "731-37-534"; SetupID = "CVOB"; break;
				case 0x98F33049: Pin = "360-30-335"; SetupID = "YXK1"; break;
				case 0x98F3304A: Pin = "236-90-738"; SetupID = "YMWW"; break;
				case 0x98F3304B: Pin = "881-48-820"; SetupID = "CC35"; break;
				case 0x98F3304C: Pin = "895-86-372"; SetupID = "TZ9B"; break;
				case 0x98F3304D: Pin = "131-37-569"; SetupID = "4INO"; break;
				case 0x98F3304E: Pin = "292-60-813"; SetupID = "KLRF"; break;
				case 0x98F3304F: Pin = "202-34-374"; SetupID = "54JY"; break;
				case 0x98F33050: Pin = "933-04-411"; SetupID = "Z978"; break;
				case 0x98F33051: Pin = "952-67-082"; SetupID = "DT19"; break;
				case 0x98F33052: Pin = "696-57-197"; SetupID = "0G81"; break;
				case 0x98F33053: Pin = "823-84-040"; SetupID = "SJ67"; break;
				case 0x98F33054: Pin = "372-17-936"; SetupID = "2B26"; break;
				case 0x98F33055: Pin = "524-33-993"; SetupID = "8638"; break;
				case 0x98F33056: Pin = "728-40-775"; SetupID = "VS49"; break;
				case 0x98F33057: Pin = "158-72-855"; SetupID = "WG63"; break;
				case 0x98F33058: Pin = "584-27-402"; SetupID = "0278"; break;
				case 0x98F33059: Pin = "867-84-622"; SetupID = "YO45"; break;
				case 0x98F3305A: Pin = "501-79-192"; SetupID = "T944"; break;
				case 0x98F3305B: Pin = "239-09-376"; SetupID = "HK47"; break;
				case 0x98F3305C: Pin = "330-92-293"; SetupID = "I478"; break;
				case 0x98F3305D: Pin = "362-17-166"; SetupID = "UK25"; break;
				case 0x98F3305E: Pin = "371-77-165"; SetupID = "TE63"; break;
				case 0x98F3305F: Pin = "336-08-468"; SetupID = "ZH64"; break;
				case 0x98F33060: Pin = "708-07-628"; SetupID = "F233"; break;
				case 0x98F33061: Pin = "691-15-657"; SetupID = "TW93"; break;
				case 0x98F33062: Pin = "953-36-842"; SetupID = "2556"; break;
				case 0x98F33063: Pin = "810-42-944"; SetupID = "E677"; break;
				case 0x98F33064: Pin = "417-70-582"; SetupID = "CJ36"; break;
				case 0x98F33065: Pin = "713-16-488"; SetupID = "2432"; break;
				case 0x98F33066: Pin = "407-28-149"; SetupID = "YJ82"; break;
				case 0x98F33067: Pin = "415-62-866"; SetupID = "E852"; break;
				case 0x98F33068: Pin = "656-18-628"; SetupID = "3Y96"; break;
				case 0x98F33069: Pin = "101-35-086"; SetupID = "EU49"; break;
				case 0x98F3306A: Pin = "736-43-083"; SetupID = "EW14"; break;
				case 0x98F3306B: Pin = "781-00-894"; SetupID = "1Y78"; break;
				case 0x98F3306C: Pin = "441-40-107"; SetupID = "W949"; break;
				case 0x98F3306D: Pin = "298-17-188"; SetupID = "T874"; break;
				case 0x98F3306E: Pin = "280-33-162"; SetupID = "PU81"; break;
				case 0x98F3306F: Pin = "564-74-043"; SetupID = "SU61"; break;
				case 0x98F33070: Pin = "693-26-344"; SetupID = "1368"; break;
				case 0x98F33071: Pin = "822-63-648"; SetupID = "P389"; break;
				case 0x98F33072: Pin = "337-00-622"; SetupID = "TS25"; break;
				case 0x98F33073: Pin = "566-54-307"; SetupID = "D637"; break;
				case 0x98F33074: Pin = "336-47-796"; SetupID = "L724"; break;
				case 0x98F33075: Pin = "908-73-402"; SetupID = "V512"; break;
				case 0x98F33076: Pin = "061-32-489"; SetupID = "FL42"; break;
				case 0x98F33077: Pin = "576-31-875"; SetupID = "EP49"; break;
				case 0x98F33078: Pin = "028-37-770"; SetupID = "7R56"; break;
				case 0x98F33079: Pin = "672-27-632"; SetupID = "CK27"; break;
				case 0x98F3307A: Pin = "107-08-510"; SetupID = "XO41"; break;
				case 0x98F3307B: Pin = "279-38-745"; SetupID = "OS28"; break;
				case 0x98F3307C: Pin = "645-42-393"; SetupID = "X691"; break;
				case 0x98F3307D: Pin = "997-00-221"; SetupID = "VP41"; break;
				case 0x98F3307E: Pin = "658-33-443"; SetupID = "YY27"; break;
				case 0x98F3307F: Pin = "469-10-808"; SetupID = "R982"; break;
				case 0x98F33080: Pin = "378-08-297"; SetupID = "S552"; break;
				case 0x98F33081: Pin = "513-28-525"; SetupID = "VI61"; break;
				case 0x98F33082: Pin = "099-70-831"; SetupID = "JBYF"; break;
				case 0x98F33083: Pin = "068-31-918"; SetupID = "G9BR"; break;
				case 0x98F33084: Pin = "444-83-414"; SetupID = "6TIK"; break;
				case 0x98F33085: Pin = "696-14-444"; SetupID = "3LNM"; break;
				case 0x98F33086: Pin = "923-34-056"; SetupID = "F87W"; break;
				case 0x98F33087: Pin = "800-75-292"; SetupID = "S2V7"; break;
				case 0x98F33088: Pin = "690-35-172"; SetupID = "HT7D"; break;
				case 0x98F33089: Pin = "038-87-380"; SetupID = "YPN3"; break;
				case 0x98F3308A: Pin = "749-10-837"; SetupID = "B8NS"; break;
				case 0x98F3308B: Pin = "867-30-147"; SetupID = "U7RW"; break;
				case 0x98F3308C: Pin = "965-59-250"; SetupID = "2NNB"; break;
				case 0x98F3308D: Pin = "756-92-476"; SetupID = "FHJT"; break;
				case 0x98F3308E: Pin = "860-96-947"; SetupID = "E3GB"; break;
				case 0x98F3308F: Pin = "084-41-294"; SetupID = "82I7"; break;
				case 0x98F33090: Pin = "918-01-879"; SetupID = "TE3Q"; break;
				case 0x98F33091: Pin = "179-73-843"; SetupID = "YYM5"; break;
				case 0x98F33092: Pin = "437-51-992"; SetupID = "0ITC"; break;
				case 0x98F33093: Pin = "936-95-528"; SetupID = "34YN"; break;
				case 0x98F33094: Pin = "166-82-105"; SetupID = "8PKO"; break;
				case 0x98F33095: Pin = "223-35-805"; SetupID = "832M"; break;
				case 0x98F33096: Pin = "604-29-457"; SetupID = "PXJ3"; break;
				case 0x98F33097: Pin = "879-78-051"; SetupID = "NAX7"; break;
				case 0x98F33098: Pin = "561-86-701"; SetupID = "TFCV"; break;
				case 0x98F33099: Pin = "919-43-976"; SetupID = "HA4Y"; break;
				case 0x98F3309A: Pin = "386-18-117"; SetupID = "JOTP"; break;
				case 0x98F3309B: Pin = "891-30-737"; SetupID = "NO0Q"; break;
				case 0x98F3309C: Pin = "913-69-914"; SetupID = "9BTW"; break;
				case 0x98F3309D: Pin = "437-13-604"; SetupID = "OPYY"; break;
				case 0x98F3309E: Pin = "322-08-103"; SetupID = "0UFR"; break;
				case 0x98F3309F: Pin = "887-33-252"; SetupID = "42DU"; break;
				case 0x98F330A0: Pin = "280-87-074"; SetupID = "VBAY"; break;
				case 0x98F330A1: Pin = "123-43-730"; SetupID = "DOGI"; break;
				case 0x98F330A2: Pin = "240-87-808"; SetupID = "AX5F"; break;
				case 0x98F330A3: Pin = "224-60-285"; SetupID = "1BJ1"; break;
				case 0x98F330A4: Pin = "642-90-909"; SetupID = "JA5P"; break;
				case 0x98F330A5: Pin = "770-50-589"; SetupID = "2J0H"; break;
				case 0x98F330A6: Pin = "804-45-605"; SetupID = "UMKQ"; break;
				case 0x98F330A7: Pin = "385-08-101"; SetupID = "NOD0"; break;
				case 0x98F330A8: Pin = "675-31-155"; SetupID = "V831"; break;
				case 0x98F330A9: Pin = "397-84-280"; SetupID = "V453"; break;
				case 0x98F330AA: Pin = "144-29-205"; SetupID = "NVYX"; break;
				case 0x98F330AB: Pin = "184-89-434"; SetupID = "8GUR"; break;
				case 0x98F330AC: Pin = "317-28-621"; SetupID = "7J73"; break;
				case 0x98F330AD: Pin = "552-90-010"; SetupID = "K12F"; break;
				case 0x98F330AE: Pin = "453-21-429"; SetupID = "CEVY"; break;
				case 0x98F330AF: Pin = "454-84-789"; SetupID = "PWYP"; break;
				case 0x98F330B0: Pin = "995-71-083"; SetupID = "8NFV"; break;
				case 0x98F330BF: Pin = "188-97-285"; SetupID = "CI80"; break;
				case 0x98F330B1: Pin = "001-33-392"; SetupID = "4NWG"; break;
				case 0x98F330B2: Pin = "901-90-100"; SetupID = "CCJT"; break;
				case 0x98F3312B: Pin = "661-73-175"; SetupID = "JWXR"; break;
				case 0x98F33187: Pin = "579-91-214"; SetupID = "2BNA"; break;
				case 0x98F3319C: Pin = "758-08-490"; SetupID = "82F6"; break;
				case 0x98F33256: Pin = "861-05-570"; SetupID = "X6TB"; break;
				case 0x98F33257: Pin = "898-05-609"; SetupID = "D1JQ"; break;
				case 0x98F33258: Pin = "741-87-988"; SetupID = "C5Y7"; break;
				case 0x98F3328F: Pin = "780-42-966"; SetupID = "R5Y6"; break;
			}
		}

		//if (IsDataSavedInNVS) {
		//	hap_enable_mfi_auth(HAP_MFI_AUTH_NONE);
		//	hap_set_setup_id("VXAE");
		//}
		//else if (Pin != "")
		if (Pin != "")
		{
		    hap_enable_mfi_auth(HAP_MFI_AUTH_NONE);
		    hap_set_setup_code(Pin.c_str());
		    hap_set_setup_id(SetupID.c_str());
//		    esp_hap_get_setup_payload(Pin.c_str(), SetupID.c_str(), false, CID);
		}
		else
		{
		    hap_enable_software_auth();
		}
	}
	else if (Mode == ModeEnum::EXPERIMENTAL)
	{
	    hap_enable_mfi_auth(HAP_MFI_AUTH_NONE);
	    hap_set_setup_code("999-55-222");
	    hap_set_setup_id("17AC");

	    esp_hap_get_setup_payload("999-55-222", "17AC", false, CID);
	}

    //::esp_event_handler_register(WIFI_EVENT	, ESP_EVENT_ANY_ID	 , &WiFi.eventHandler, &WiFi);
    //::esp_event_handler_register(IP_EVENT	, IP_EVENT_STA_GOT_IP, &WiFi.eventHandler, &WiFi);

    //app_wifi_init();

    /* After all the initializations are done, start the HAP core */
    if (hap_start() == HAP_SUCCESS)
    {
        //app_wifi_start(portMAX_DELAY);
        httpd_handle_t *HAPWebServerHandle = hap_platform_httpd_get_handle();
        if (HAPWebServerHandle != NULL)
        	WebServer.RegisterHandlers(*HAPWebServerHandle);
    }
    else
    	WebServer.HTTPStart();


    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */

    vTaskDelete(NULL);
}
