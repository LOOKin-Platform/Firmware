#include "HomeKit.h"

const char *Tag = "HAP Bridge";

TaskHandle_t 		HomeKit::TaskHandle = NULL;

bool 				HomeKit::IsRunning 		= false;
bool 				HomeKit::IsAP 			= false;
HomeKit::ModeEnum	HomeKit::Mode			= HomeKit::ModeEnum::NONE;

uint64_t			HomeKit::VolumeLastUpdated = 0;

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
}

void HomeKit::Start() {
	if (TaskHandle == NULL)
		TaskHandle = FreeRTOS::StartTask(Task, "hap_bridge", NULL, 4096, 1);
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

    	ESP_LOGE("ON", "UUID: %04X, Value: %d", AID, Value);

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.DeviceType == 0x01 && (Time::UptimeU() - VolumeLastUpdated) < 1000000)
        	return HAP_STATUS_SUCCESS;

        if (IRDeviceItem.IsEmpty())
        	return HAP_STATUS_VAL_INVALID;

        if (IRDeviceItem.DeviceType == 0xEF) { // air conditioner
        	// check service. If fan for ac - skip
        	if (strcmp(hap_serv_get_type_uuid(hap_char_get_parent(Char)), HAP_SERV_UUID_FAN_V2) == 0)
        		return HAP_STATUS_SUCCESS;

        	uint8_t NewValue = 0;
        	uint8_t CurrentMode = ((DataRemote_t*)Data)->DevicesHelper.GetDeviceForType(0xEF)->GetStatusByte(IRDeviceItem.Status , 0);

        	if (Value)
        	{
        		const hap_val_t *ValueForCurrentState = hap_char_get_val(hap_serv_get_char_by_uuid(hap_char_get_parent(Char), HAP_CHAR_UUID_TARGET_HEATER_COOLER_STATE));

                switch (ValueForCurrentState->u) {
                	case 0: NewValue = 1; break;
                	case 1: NewValue = 3; break;
                	case 2: NewValue = 2; break;
                }
        	}

        	if ((!Value && CurrentMode > 0) || (Value && CurrentMode == 0))
        		StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, NewValue);

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
	ESP_LOGE("Cursor for UUID", "%s", UUID.c_str());

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
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
    	VolumeLastUpdated = Time::UptimeU();

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

        // change temeprature because of it may depends on selected mode
        const hap_val_t* NewTempValue  = HomeKitGetCharValue(AID, HAP_SERV_UUID_HEATER_COOLER,
        		(Value == 3) ? HAP_CHAR_UUID_HEATING_THRESHOLD_TEMPERATURE : HAP_CHAR_UUID_COOLING_THRESHOLD_TEMPERATURE);

        if (NewTempValue != NULL)
            StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra, 0xE1, round(NewTempValue->f), false);


        // change current heater cooler state
        hap_val_t CurrentHeaterCoolerState;
        CurrentHeaterCoolerState.u =  (Value == 3) ? 2 : 3;
        HomeKitUpdateCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_HEATER_COOLER_STATE, CurrentHeaterCoolerState);

    	if (Value > 0) {
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
	ESP_LOGE("TargetFanState", "UUID: %04X, Value: %d", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	if (Iterator > 0) return true;

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
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

	if (!Send) return;

	if (!Result.first) return;
	if (Codeset == 0) return;

    CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

    if (IRCommand == nullptr) return;

    string Operand = Converter::ToHexString(Codeset,4) + Converter::ToHexString(Result.second,4);
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

            	if (*(write->status) == HAP_STATUS_SUCCESS && IRDeviceItem.DeviceType == 0x01) {
        			static hap_val_t HAPValue;
        			HAPValue.u = (uint8_t)write->val.b;
        			HomeKitUpdateCharValue(AID, SERVICE_TV_UUID, CHAR_SLEEP_DISCOVERY_UUID, HAPValue);
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
		if (NameString == "")
			NameString = Device.TypeToString() + " " + Device.IDToString();

		static AccessoryData_t AccessoryData(NameString, Device.ModelToString(), Device.IDToString());

		hap_acc_cfg_t cfg = {
			.name 				= AccessoryData.Name,
			.model 				= AccessoryData.Model,
			.manufacturer 		= "LOOK.in",
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
		if (BridgeNameString == "") BridgeNameString = Device.TypeToString() + " " + Device.IDToString();

		static AccessoryData_t AccessoryData(BridgeNameString, Device.ModelToString(), Device.IDToString());

		hap_acc_cfg_t cfg = {
			.name 				= AccessoryData.Name,
			.model 				= AccessoryData.Model,
			.manufacturer 		= "LOOK.in",
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

		char accessory_name[32] = {0};

		string Name = IRDevice.Name;

		if (Name.size() > 32)
			Name = Name.substr(0, 32);

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
		// first Remote2 batch fix

		string 	Pin 	= "";
		string	SetupID = "";

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
		}


		if (Pin != "") {
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
