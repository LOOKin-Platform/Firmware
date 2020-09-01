#include "HomeKit.h"
#include "Custom.cpp"

const char *Tag = "HAP Bridge";

TaskHandle_t 	HomeKit::TaskHandle = NULL;

bool 			HomeKit::IsRunning 	= false;
bool 			HomeKit::IsAP 		= false;
string			HomeKit::SSID		= "";
string			HomeKit::Password	= "";

vector<hap_acc_t*> 		HomeKit::BridgedAccessories = vector<hap_acc_t*>();
map<uint16_t, uint64_t> HomeKit::LastUpdated 		= map<uint16_t, uint64_t>();

#define NUM_BRIDGED_ACCESSORIES 2


/* Setup Information for the Setup Code: 111-22-333 */
static const hap_setup_info_t setup_info = {
    .salt = {
        0x93, 0x15, 0x1A, 0x47, 0x57, 0x55, 0x3C, 0x21, 0x0B, 0x55, 0x89, 0xB8, 0xC3, 0x99, 0xA0, 0xF3
    },
    .verifier = {
        0x9E, 0x9C, 0xC3, 0x73, 0x9B, 0x04, 0x83, 0xC8, 0x13, 0x7C, 0x5B, 0x5F, 0xAC, 0xC5, 0x63, 0xDF,
        0xF4, 0xF1, 0x0F, 0x39, 0x06, 0x4A, 0x20, 0x2D, 0x53, 0x2A, 0x09, 0x20, 0x3A, 0xA6, 0xBA, 0xE3,
        0x1E, 0x42, 0x4E, 0x58, 0x4E, 0xBB, 0x44, 0x5F, 0x7F, 0xDF, 0xCC, 0x11, 0xD0, 0xF7, 0x8B, 0x35,
        0xE1, 0x16, 0xA9, 0x79, 0x30, 0xBC, 0x37, 0x19, 0x77, 0x36, 0xB1, 0xEC, 0xD4, 0x12, 0x4C, 0xE4,
        0x5D, 0xE3, 0x7E, 0x46, 0xA0, 0x2D, 0x10, 0x07, 0xAB, 0x48, 0x40, 0x36, 0xD5, 0x3F, 0x7F, 0xBE,
        0xA5, 0xAE, 0xD0, 0x25, 0x6B, 0xC4, 0x9E, 0xC8, 0x5F, 0xC9, 0x4E, 0x47, 0x0D, 0xBA, 0xD3, 0x63,
        0x44, 0x20, 0x01, 0x69, 0x97, 0xDD, 0x20, 0x54, 0x7C, 0x59, 0x78, 0x3D, 0x5C, 0x6D, 0xC7, 0x1F,
        0xE6, 0xFD, 0xA0, 0x8E, 0x9B, 0x36, 0x45, 0x1F, 0xC1, 0x4B, 0xB5, 0x26, 0xE1, 0x8E, 0xEB, 0x4C,
        0x05, 0x58, 0xD7, 0xC8, 0x80, 0xA1, 0x43, 0x7F, 0x5F, 0xDB, 0x75, 0x1B, 0x19, 0x57, 0x25, 0xAC,
        0x5D, 0xF5, 0x8D, 0xF6, 0x7B, 0xAA, 0xB7, 0x7D, 0xE0, 0x36, 0xEF, 0xEA, 0xF3, 0x57, 0xAC, 0xFE,
        0x12, 0x87, 0xF9, 0x31, 0x4C, 0xF7, 0x44, 0xBD, 0xB6, 0x26, 0x6C, 0xB4, 0x0D, 0x7C, 0x52, 0x4F,
        0x85, 0x56, 0x91, 0x5D, 0x13, 0xD8, 0xDA, 0x8C, 0x45, 0x3E, 0x73, 0xF2, 0xF9, 0x20, 0x39, 0x24,
        0x8B, 0xFB, 0xEE, 0xFD, 0x77, 0x54, 0x8D, 0x37, 0x22, 0xE8, 0x55, 0xC3, 0xD2, 0xF8, 0xB8, 0x23,
        0xB0, 0xE2, 0x9E, 0x43, 0xAE, 0xB4, 0x37, 0xFA, 0xA7, 0x03, 0xF1, 0x82, 0x68, 0x4C, 0xD4, 0x86,
        0xC6, 0x3E, 0xDE, 0x70, 0x11, 0x03, 0x77, 0x46, 0x59, 0x14, 0x97, 0xC6, 0xAE, 0x52, 0x6F, 0x03,
        0x77, 0x36, 0x40, 0xBC, 0xDE, 0xCD, 0x3D, 0xE0, 0x4F, 0x69, 0x18, 0x0D, 0xCA, 0x85, 0x7E, 0x07,
        0x30, 0xF4, 0xA1, 0xCE, 0x05, 0xB5, 0x4B, 0xE1, 0x1D, 0x43, 0xDF, 0xDB, 0x11, 0x43, 0xDE, 0x21,
        0xAC, 0x8F, 0x03, 0x9E, 0x6E, 0x9F, 0xA8, 0xE5, 0x02, 0x06, 0x1C, 0x63, 0x34, 0x22, 0x1D, 0x39,
        0xE3, 0x3D, 0x12, 0x2E, 0xA2, 0xF3, 0xFC, 0xB5, 0xB4, 0x16, 0x9E, 0x0E, 0x7C, 0x52, 0xC8, 0x7D,
        0x50, 0x3D, 0xDB, 0xF5, 0x83, 0x46, 0x18, 0x92, 0x7F, 0x4D, 0x38, 0xAD, 0x0A, 0x2A, 0xBC, 0x2A,
        0x50, 0x4B, 0xDF, 0x5D, 0xFA, 0x93, 0x41, 0x78, 0xD6, 0x45, 0x54, 0xDB, 0x44, 0x81, 0xF7, 0x5A,
        0x0A, 0xDD, 0x18, 0x4F, 0x27, 0xD7, 0xDD, 0x5E, 0xB7, 0x3E, 0x99, 0xE6, 0xE1, 0x69, 0x35, 0x74,
        0xD6, 0x98, 0x58, 0xB2, 0x13, 0x6F, 0xB7, 0x82, 0x72, 0xBC, 0xA6, 0x8B, 0xA3, 0x36, 0x2A, 0xCE,
        0x65, 0x65, 0x51, 0x08, 0x8A, 0x3D, 0x04, 0x93, 0x8F, 0x01, 0x8A, 0xAB, 0x4B, 0xFC, 0x06, 0xF9
    }
};

HomeKit::AccessoryData_t::AccessoryData_t(string sName, string sModel, string sID) {
	//sName.copy(Name, sName.size(), 0);

	if (sName.size() > 32) sName 	= sName.substr(0, 32);
	if (sModel.size() > 2) sModel 	= sModel.substr(0, 32);
	if (sID.size() > 8) 	sID 	= sID.substr(0, 32);

	::sprintf(Name	, sName.c_str());
	::sprintf(Model	, sModel.c_str());
	::sprintf(ID	, sID.c_str());
}

void HomeKit::WiFiSetMode(bool sIsAP, string sSSID, string sPassword) {
	IsAP 		= sIsAP;
	SSID 		= sSSID;
	Password 	= sPassword;
}

void HomeKit::Start() {
	if (TaskHandle == NULL)
		TaskHandle = FreeRTOS::StartTask(Task, "hap_bridge", NULL, 4096, 1);
}


void HomeKit::Stop() {
	::hap_stop();
}

void HomeKit::AppServerRestart() {
	LastUpdated.clear();
	FillAccessories();
	return;
}

void HomeKit::ResetData() {
	::hap_reset_homekit_data();
}


/* Mandatory identify routine for the accessory (bridge)
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
int HomeKit::BridgeIdentify(hap_acc_t *ha) {
    ESP_LOGI(Tag, "Bridge identified");
    return HAP_SUCCESS;
}

/* Mandatory identify routine for the bridged accessory
 * In a real bridge, the actual accessory must be sent some request to
 * identify itself visually
 */
int HomeKit::AccessoryIdentify(hap_acc_t *ha)
{
    hap_serv_t *hs = hap_acc_get_serv_by_uuid(ha, HAP_SERV_UUID_ACCESSORY_INFORMATION);
    hap_char_t *hc = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_NAME);
    const hap_val_t *val = hap_char_get_val(hc);
    char *name = val->s;

    ESP_LOGI(Tag, "Bridged Accessory %s identified", name);
    return HAP_SUCCESS;
}

bool HomeKit::On(bool Value, uint16_t AID, uint8_t Iterator) {

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	if (Iterator > 0) return true;

    	ESP_LOGE("ON", "UUID: %04X, Value: %d", AID, Value);

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty())
        	return false;

        if (IRDeviceItem.DeviceType == 0xEF) { // air conditionair

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
        	return true;
        }
        else
        {
        	if (Functions.count("poweron") > 0 && Value)
        	{
            	Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("poweron"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
            	return true;

        	}

        	if (Functions.count("poweroff") > 0 && !Value)
        	{
            	Operand += Converter::ToHexString(((DataRemote_t*)Data)->DevicesHelper.FunctionIDByName("poweroff"), 2) + "FF";
        		IRCommand->Execute(0xFE, Operand.c_str());
            	return true;
        	}
        }
    }

    return false;
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

bool HomeKit::HeatingCoolingState(uint8_t Value, uint16_t AID) {
	ESP_LOGE("HeaterCoolerState", "UUID: %04X, Value: %d", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
        	return false;

        switch (Value)
        {
        	case 1: Value = 3; break;
        	case 2:	Value = 2; break;
        	case 3: Value = 1; break;
        	default: break;
        }

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, Value);

        return true;
    }

    return false;
}

bool HomeKit::TargetTemperature(float Value, uint16_t AID) {
	ESP_LOGE("TargetTemperature", "UUID: %04X, Value: %f", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
        	return false;

        if (Value > 30) Value = 30;
        if (Value < 16) Value = 16;

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE1, round(Value));
        return true;
    }

    return false;
}

bool HomeKit::RotationSpeed(float Value, uint16_t AID, hap_char_t *Char, uint8_t Iterator) {
	ESP_LOGE("RotationSpeed", "UUID: %04X, Value: %f", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
    	if (Iterator > 0) return true;

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // Air Conditionair
        	return false;

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE2, round(Value));

        hap_val_t ValueForTargetState;
        ValueForTargetState.u = (Value == 0) ? 1 : 0;
        hap_char_update_val(hap_serv_get_char_by_uuid(hap_char_get_parent(Char), HAP_CHAR_UUID_TARGET_FAN_STATE), &ValueForTargetState);

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

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE2, (Value) ? 0 : 2);

        hap_val_t ValueForRotationSpeed;
        ValueForRotationSpeed.f = (Value == false) ? 2.0 : 0.0;
        hap_char_update_val(hap_serv_get_char_by_uuid(hap_char_get_parent(Char), HAP_CHAR_UUID_ROTATION_SPEED), &ValueForRotationSpeed);

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


void HomeKit::StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value) {
	pair<bool, uint16_t> Result = ((DataRemote_t*)Data)->StatusUpdateForDevice(UUID, FunctionID, Value);

	if (!Result.first) return;

	ESP_LOGE("New Status", "%04X", Result.second);

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
    ESP_LOGI(Tag, "Write called for Accessory with AID %04X", AID);

    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];

        ESP_LOGI(Tag, "Characteristic: %s, i: %d", hap_char_get_type_uuid(write->hc), i);

        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
        	On(write->val.b, AID, i);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ACTIVE)) {
        	On(write->val.b, AID, i);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_REMOTEKEY_UUID)) {
        	Cursor(write->val.b, AID);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_ACTIVE_IDENTIFIER_UUID)) {
        	ActiveID(write->val.b, AID);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), CHAR_VOLUME_SELECTOR_UUID)) {
        	Volume(write->val.b, AID);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_HEATING_COOLING_STATE)) {
        	HeatingCoolingState(write->val.b, AID);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_TEMPERATURE)) {
        	TargetTemperature(write->val.f, AID);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ROTATION_SPEED)) {
        	RotationSpeed(write->val.f, AID, write->hc, i);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_FAN_STATE)) {
        	TargetFanState(write->val.b, AID, write->hc, i);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SWING_MODE)) {
        	SwingMode(write->val.b, AID, write->hc, i);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
        }
    }

    SetLastUpdatedForAID(AID);

    return ret;
}

void HomeKit::FillAccessories() {
	hap_acc_t 	*Accessory;

	if (Settings.eFuse.Type == 0x81) {
		if (BridgedAccessories.size() == 0) {
			string BridgeNameString = Device.GetName();
			if (BridgeNameString == "") BridgeNameString = Device.TypeToString() + " " + Device.IDToString();

			static AccessoryData_t AccessoryData(BridgeNameString, Device.ModelToString(), Device.IDToString());

			hap_acc_cfg_t cfg = {
				.name 				= AccessoryData.Name,
				.model 				= AccessoryData.Model,
				.manufacturer 		= "LOOK.in",
				.serial_num 		= AccessoryData.ID,
				.fw_rev 			= (char*)Settings.FirmwareVersion,
				.hw_rev 			= NULL,
				.pv 				= "1.1.0",
				.cid 				= HAP_CID_BRIDGE,
				.identify_routine 	= BridgeIdentify,
			};

			/* Create accessory object */
			Accessory = hap_acc_create(&cfg);

			/* Product Data as per HAP Spec R15. Please use the actual product data
			 * value assigned to your Product Plan
			 */

			uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
			hap_acc_add_product_data(Accessory, product_data, sizeof(product_data));

			/* Add the Accessory to the HomeKit Database */
			hap_add_accessory(Accessory);
		}

		for(auto& BridgeAccessory : BridgedAccessories) {
			hap_remove_bridged_accessory(BridgeAccessory);
			hap_acc_delete(BridgeAccessory);
		}

		BridgedAccessories.clear();

		for (auto &IRDevice : ((DataRemote_t *)Data)->GetAvaliableDevices())
		{
			char accessory_name[10] = "Accessory";
//			sprintf(accessory_name, "Accessory");

			hap_acc_cfg_t bridge_cfg = {
				.name 				= accessory_name,
				.model 				= NULL,
				.manufacturer 		= NULL,
				.serial_num 		= "n/a",
				.fw_rev 			= (char*)Settings.FirmwareVersion,
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
					hap_serv_t *ServiceTV;
					ServiceTV = hap_serv_tv_create(false);
					hap_serv_add_char		(ServiceTV, hap_char_name_create(accessory_name));

					hap_serv_set_priv		(ServiceTV, (void *)(uint32_t)AID);
					hap_serv_set_write_cb	(ServiceTV, WriteCallback);
					hap_acc_add_serv		(Accessory, ServiceTV);

					if (IRDevice.Functions.count("volup") > 0 || IRDevice.Functions.count("voldown") > 0) {
						hap_serv_t *ServiceSpeaker;
						ServiceSpeaker = hap_serv_tv_speaker_create(false);
						hap_serv_add_char		(ServiceSpeaker, hap_char_name_create(accessory_name));

						hap_serv_set_priv		(ServiceSpeaker, (void *)(uint32_t)AID);
						hap_serv_set_write_cb	(ServiceSpeaker, WriteCallback);
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
						hap_serv_link_serv(ServiceTV, ServiceInput);
						hap_acc_add_serv(Accessory, ServiceInput);
					}

					break;

				case 0x03: // light
					hap_serv_t *ServiceLight;
					ServiceLight = hap_serv_lightbulb_create(false);
					hap_serv_add_char(ServiceLight, hap_char_name_create(accessory_name));

					hap_serv_set_priv(ServiceLight, (void *)(uint32_t)AID);
					hap_serv_set_write_cb(ServiceLight, WriteCallback);
					hap_acc_add_serv(Accessory, ServiceLight);

					break;

				case 0x07: // Fan
					hap_serv_t *ServiceFan;
					ServiceFan = hap_serv_fan_v2_create(false);
					hap_serv_add_char(ServiceFan, hap_char_name_create(accessory_name));

					hap_serv_set_priv(ServiceFan, (void *)(uint32_t)AID);
					hap_serv_set_write_cb(ServiceFan, WriteCallback);
					hap_acc_add_serv(Accessory, ServiceFan);
					break;

				case 0xEF: { // AC
					ACOperand Operand((uint32_t)IRDevice.Status);

					if 		(Operand.Mode == 3) Operand.Mode = 1;
					else if (Operand.Mode == 1) Operand.Mode = 3;
					else if (Operand.Mode  > 3) Operand.Mode = 0x0;

					ESP_LOGE("hap_serv_ac_tempmode_create", "(%d, %d)", Operand.Mode, Operand.Temperature);

					hap_serv_t *ServiceAC;
					ServiceAC = hap_serv_ac_tempmode_create(Operand.Mode, Operand.Mode, Operand.Temperature, Operand.Temperature, 0);
					hap_serv_add_char(ServiceAC, hap_char_name_create("Air Conditioner"));
					hap_serv_set_priv(ServiceAC, (void *)(uint32_t)AID);
					hap_serv_set_write_cb(ServiceAC, WriteCallback);
					hap_acc_add_serv(Accessory, ServiceAC);

					ESP_LOGE("hap_serv_ac_fanswing_create", "(%d, %d, %d)", (Operand.FanMode == 0) ? 1 : 0, Operand.SwingMode, Operand.FanMode);

					hap_serv_t *ServiceACFan;
					ServiceACFan = hap_serv_ac_fanswing_create((Operand.FanMode == 0) ? 1 : 0, Operand.SwingMode, Operand.FanMode);
					hap_serv_add_char(ServiceACFan, hap_char_name_create("Swing & Ventillation"));

					hap_serv_set_priv(ServiceACFan, (void *)(uint32_t)AID);
					hap_serv_set_write_cb(ServiceACFan, WriteCallback);
					hap_serv_link_serv(ServiceAC, ServiceACFan);
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
	}
}


void HomeKit::Task(void *) {
	IsRunning = true;

	/* Initialize the HAP core */
	hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    //sprintf(accessory_name, "ESP-Fan-%d", i);


	FillAccessories();

    /* Set the Setup ID required for QR code based Accessory Setup.
     * Ideally, this should be available in factory_nvs partition. This is
     * just for demonstration purpose
     */
	hap_set_setup_id("ES32");

    /* Provide the Setup Information for HomeKit Pairing.
     * Ideally, this should be available in factory_nvs partition. This is
     * just for demonstration purpose
     */
	hap_set_setup_info(&setup_info);

    /* Register a common button for reset Wi-Fi network and reset to factory.
     */
    //reset_key_init(RESET_GPIO);

    /* Use the setup_payload_gen tool to get the QR code for Accessory Setup.
     * The payload below is for a Bridge with setup code 111-22-333 and setup id ES32
     */
    ESP_LOGI(Tag, "Use setup payload: \"X-HM://002LETYN1ES32\" for Accessory Setup");

    /* Enable WAC2 as per HAP Spec R12 */
    hap_enable_wac2();

    ::hap_set_softap_ssid(Settings.WiFi.APSSID.c_str());

    if (!IsAP && SSID != "" && Password != "") {
#if CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL
    	::hap_set_wifi_network_ext(SSID.c_str(), SSID.size(), Password.c_str(), Password.size());
#endif

#if CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED
    	::hap_disable_mfi_auth(SSID.c_str(), Password.c_str());
#endif
    }

    ::esp_event_handler_register(WIFI_EVENT	, ESP_EVENT_ANY_ID	 , &WiFi.eventHandler, &WiFi);
    ::esp_event_handler_register(IP_EVENT	, IP_EVENT_STA_GOT_IP, &WiFi.eventHandler, &WiFi);

    /* After all the initializations are done, start the HAP core */
    hap_start();

    WebServer.RegisterHandlers(*hap_platform_httpd_get_handle());

    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */

    vTaskDelete(NULL);
}

void HomeKit::SetLastUpdatedForAID(uint16_t AID) {
	LastUpdated[AID] = Time::UptimeU();
}

uint64_t HomeKit::GetLastUpdatedForAID(uint16_t AID) {
	return (LastUpdated.count(AID) > 0) ? LastUpdated[AID] : 0;
}

bool HomeKit::IsRecentAction(uint16_t AID) {
	ESP_LOGE("IsRecentAction", "%" PRIu64 " %" PRIu64, GetLastUpdatedForAID(AID), Time::UptimeU());

	return (Time::UptimeU() - GetLastUpdatedForAID(AID) < 500);
}

