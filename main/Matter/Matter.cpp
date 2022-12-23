#include "Matter.h"
#include "MatterDevice.h"
#include "DeviceCallbacks.h"
#include "NetworkCommissioningCustomDriver.h"
#include "MatterWiFi.h"

#include <app-common/zap-generated/af-structs.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/reporting/reporting.h>
#include <app/util/attribute-storage.h>
#include <common/Esp32AppServer.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPMemString.h>
#include <lib/support/ErrorStr.h>
#include <lib/support/ZclString.h>

#include <app/server/OnboardingCodesUtil.h>

#include <app/server/Dnssd.h>

#include <app/InteractionModelEngine.h>
#include <app/server/Server.h>

#include <esp_log.h>

#include "MatterWiFi.h"

#if CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER
#include <platform/ESP32/ESP32FactoryDataProvider.h>
#endif // CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER

#if CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER
#include <platform/ESP32/ESP32DeviceInfoProvider.h>
#else
#include <DeviceInfoProviderImpl.h>
#endif // CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER

namespace {
#if CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER
chip::DeviceLayer::ESP32FactoryDataProvider sFactoryDataProvider;
#endif // CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER

#if CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER
chip::DeviceLayer::ESP32DeviceInfoProvider gExampleDeviceInfoProvider;
#else
chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
#endif // CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER
} // namespace


using namespace ::chip;
using namespace ::chip::DeviceManager;
using namespace ::chip::DeviceLayer;
using namespace ::chip::Platform;
using namespace ::chip::Credentials;
using namespace ::chip::app::Clusters;

static AppDeviceCallbacks AppCallback;
//static AppDeviceCallbacksDelegate sAppDeviceCallbacksDelegate;

const char *Tag = "Matter";

TaskHandle_t 		Matter::TaskHandle = NULL;

bool 				Matter::IsRunning 		= false;
bool 				Matter::IsAP 			= false;

uint64_t			Matter::TVHIDLastUpdated = 0;

vector<uint8_t*> 	Matter::BridgedAccessories = vector<uint8_t*>();

static const int kNodeLabelSize = 32;
// Current ZCL implementation of Struct uses a max-size array of 254 bytes
static const int kDescriptorAttributeArraySize = 254;

// Declare On/Off cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(onOffAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(ZCL_ON_OFF_ATTRIBUTE_ID, BOOLEAN, 1, 0), /* on/off */
    DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Descriptor cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(descriptorAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(ZCL_DEVICE_LIST_ATTRIBUTE_ID, ARRAY, kDescriptorAttributeArraySize, 0),     /* device list */
    DECLARE_DYNAMIC_ATTRIBUTE(ZCL_SERVER_LIST_ATTRIBUTE_ID, ARRAY, kDescriptorAttributeArraySize, 0), /* server list */
    DECLARE_DYNAMIC_ATTRIBUTE(ZCL_CLIENT_LIST_ATTRIBUTE_ID, ARRAY, kDescriptorAttributeArraySize, 0), /* client list */
    DECLARE_DYNAMIC_ATTRIBUTE(ZCL_PARTS_LIST_ATTRIBUTE_ID, ARRAY, kDescriptorAttributeArraySize, 0),  /* parts list */
    DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Bridged Device Basic information cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(bridgedDeviceBasicAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(ZCL_NODE_LABEL_ATTRIBUTE_ID, CHAR_STRING, kNodeLabelSize, 0), /* NodeLabel */
    DECLARE_DYNAMIC_ATTRIBUTE(ZCL_REACHABLE_ATTRIBUTE_ID, BOOLEAN, 1, 0),               /* Reachable */
    DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Cluster List for Bridged Light endpoint
// TODO: It's not clear whether it would be better to get the command lists from
// the ZAP config on our last fixed endpoint instead.
constexpr CommandId onOffIncomingCommands[] = {
    app::Clusters::OnOff::Commands::Off::Id,
    app::Clusters::OnOff::Commands::On::Id,
    app::Clusters::OnOff::Commands::Toggle::Id,
    app::Clusters::OnOff::Commands::OffWithEffect::Id,
    app::Clusters::OnOff::Commands::OnWithRecallGlobalScene::Id,
    app::Clusters::OnOff::Commands::OnWithTimedOff::Id,
    kInvalidCommandId,
};

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedLightClusters)
DECLARE_DYNAMIC_CLUSTER(ZCL_ON_OFF_CLUSTER_ID, onOffAttrs, onOffIncomingCommands, nullptr),
    DECLARE_DYNAMIC_CLUSTER(ZCL_DESCRIPTOR_CLUSTER_ID, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(ZCL_BRIDGED_DEVICE_BASIC_CLUSTER_ID, bridgedDeviceBasicAttrs, nullptr,
                            nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Light endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedLightEndpoint, bridgedLightClusters);


DataVersion gLight1DataVersions[ArraySize(bridgedLightClusters)];
DataVersion gLight2DataVersions[ArraySize(bridgedLightClusters)];
DataVersion gLight3DataVersions[ArraySize(bridgedLightClusters)];
DataVersion gLight4DataVersions[ArraySize(bridgedLightClusters)];

static EndpointId gCurrentEndpointId;
static EndpointId gFirstDynamicEndpointId;

static MatterDevice * gDevices[CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT]; // number of dynamic endpoints count

#define DEVICE_TYPE_BRIDGED_NODE 0x0013
#define DEVICE_TYPE_LO_ON_OFF_LIGHT 0x0100

#define DEVICE_TYPE_ROOT_NODE 0x0016
#define DEVICE_TYPE_BRIDGE 0x000e

#define DEVICE_VERSION_DEFAULT 1

const EmberAfDeviceType gRootDeviceTypes[] 			= { { DEVICE_TYPE_ROOT_NODE, DEVICE_VERSION_DEFAULT } };
const EmberAfDeviceType gAggregateNodeDeviceTypes[] = { { DEVICE_TYPE_BRIDGE, DEVICE_VERSION_DEFAULT } };

const EmberAfDeviceType gBridgedOnOffDeviceTypes[] = { { DEVICE_TYPE_LO_ON_OFF_LIGHT, DEVICE_VERSION_DEFAULT },
                                                       { DEVICE_TYPE_BRIDGED_NODE, DEVICE_VERSION_DEFAULT } };


/*
Matter::AccessoryData_t(string sName, string sModel, string sID) {
	//sName.copy(Name, sName.size(), 0);

	Converter::FindAndRemove(sName, "., ");

	if (sName.size() > 32) sName 	= sName.substr(0, 32);
	if (sModel.size() > 2) sModel 	= sModel.substr(0, 32);
	if (sID.size() > 8) 	sID 	= sID.substr(0, 32);

	::sprintf(Name	, sName.c_str());
	::sprintf(Model	, sModel.c_str());
	::sprintf(ID	, sID.c_str());
}
*/

void Matter::WiFiSetMode(bool sIsAP, string sSSID, string sPassword) {
	IsAP 		= sIsAP;
}

static MatterDevice gLight1("Light 1", "Office");
static MatterDevice gLight2("Light 2", "Office");
static MatterDevice gLight3("Light 3", "Kitchen");
static MatterDevice gLight4("Light 4", "Den");

void Matter::Init() {
	memset(gDevices, 0, sizeof(gDevices));

    gLight1.SetReachable(true);
    gLight2.SetReachable(true);
    gLight3.SetReachable(true);
    gLight4.SetReachable(true);

    // Whenever bridged device changes its state
    gLight1.SetChangeCallback(&Matter::HandleDeviceStatusChanged);
    gLight2.SetChangeCallback(&Matter::HandleDeviceStatusChanged);
    gLight3.SetChangeCallback(&Matter::HandleDeviceStatusChanged);
    gLight4.SetChangeCallback(&Matter::HandleDeviceStatusChanged);

	DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    CHIPDeviceManager & deviceMgr = CHIPDeviceManager::GetInstance();

    CHIP_ERROR chip_err = deviceMgr.Init(&AppCallback);
    if (chip_err != CHIP_NO_ERROR)
    {
        ESP_LOGE(Tag, "device.Init() failed: %s", ErrorStr(chip_err));
        return;
    }

#if CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER
    SetCommissionableDataProvider(&sFactoryDataProvider);
    SetDeviceAttestationCredentialsProvider(&sFactoryDataProvider);
#if CONFIG_ENABLE_ESP32_DEVICE_INSTANCE_INFO_PROVIDER
    SetDeviceInstanceInfoProvider(&sFactoryDataProvider);
#endif
#else
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif // CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER

//  chip::DeviceLayer::PlatformMgr().ScheduleWork(Matter::InitServer, reinterpret_cast<intptr_t>(nullptr));
}

app::Clusters::NetworkCommissioning::Instance
    sWiFiNetworkCommissioningInstance(0 /* Endpoint Id */, &(chip::DeviceLayer::NetworkCommissioning::ESPCustomWiFiDriver::GetInstance()));

//app::Clusters::NetworkCommissioning::Instance
//    sWiFiNetworkCommissioningInstance(0 /* Endpoint Id */, &(chip::DeviceLayer::NetworkCommissioning::ESPWiFiDriver::GetInstance()));

void Matter::StartServer() {
	//esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, PlatformManagerImpl::HandleESPSystemEvent, NULL);
	//esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, PlatformManagerImpl::HandleESPSystemEvent, NULL);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(StartServerInner, reinterpret_cast<intptr_t>(nullptr));
}

void Matter::WiFiScan() {
	wifi_mode_t *mode = {0};
	esp_err_t err = esp_wifi_get_mode(mode);

	bool IsWiFiInited = !(err == ESP_ERR_WIFI_NOT_INIT);

	if (!IsWiFiInited) {
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		::esp_wifi_init(&cfg);
	}

	chip::ByteSpan Param = {};

	//! hack, но без него сразу же начинает коннектится
	::esp_wifi_disconnect();

	m_scanFinished.Take("MScanFinished");
	ESP_LOGE("WiFiScan", "MScanFinished TAKE");

	//! перенести параметры скана из Wifi.h - сейчас сделано через хардкод в Matter
	chip::DeviceLayer::NetworkCommissioning::WiFiDriver::ScanCallback *Callback = new MatterWiFi();
	chip::DeviceLayer::NetworkCommissioning::ESPWiFiDriver::GetInstance().ScanNetworks(Param, Callback);

	if (!IsWiFiInited) {
		::esp_wifi_deinit();
	}

	m_scanFinished.Wait();
}

void Matter::WiFiConnectToAP(string SSID, string Password) {
	chip::DeviceLayer::NetworkCommissioning::ESPWiFiDriver::GetInstance().ConnectWiFiNetwork(SSID.c_str(), SSID.size(), Password.c_str(), Password.size());
}

void Matter::StartServerInner(intptr_t context) {
	Esp32AppServer::Init();

    PlatformMgr().AddEventHandler(DeviceEventCallback, reinterpret_cast<intptr_t>(nullptr));

	CreateAccessories(context);
}


void Matter::Start() {
    //chip::app::DnssdServer::Instance().StartServer();
}

void Matter::Stop() {
	//!if (IsRunning)
	//!	::hap_stop();
}

void Matter::AppServerRestart() {
	Stop();
	//CreateAccessories();
	Start();
	return;
}

void Matter::ResetPairs() {
	//!::hap_reset_pairings();
}

void Matter::ResetData() {
	chip::Server::GetInstance().ScheduleFactoryReset();
}

bool Matter::IsEnabledForDevice() {
	return true;
}

void Matter::Reboot() {
	//!hap_reboot_accessory();
}

/* Mandatory identify routine for the accessory (bridge)
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
int Matter::BridgeIdentify() {
    ESP_LOGI(Tag, "Bridge identified");
    Log::Add(Log::Events::Misc::HomeKitIdentify);
    return 0;
}

/* Mandatory identify routine for the bridged accessory
 * In a real bridge, the actual accessory must be sent some request to
 * identify itself visually
 */
int Matter::AccessoryIdentify(uint16_t AID)
{
	//!ESP_LOGI(Tag, "Bridge identified");
    //!Log::Add(Log::Events::Misc::HomeKitIdentify);
    //!return HAP_SUCCESS;

	return 0;
}

bool Matter::On(bool Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return 0;

	/*
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

        	if (Value == 0) { // off
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

			if (Functions.count("poweron") > 0 || Functions.count("poweroff") > 0)
            	return HAP_STATUS_SUCCESS;
        }
    }

    return HAP_STATUS_VAL_INVALID;
	*/
}

bool Matter::Cursor(uint8_t Value, uint16_t AccessoryID) {
	return true;

	/*
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
	*/
}

bool Matter::ActiveID(uint8_t NewActiveID, uint16_t AccessoryID) {
	return true;

	/*
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
	*/
}

bool Matter::Volume(uint8_t Value, uint16_t AccessoryID) {
	return false;

	/*
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
	*/
}

bool Matter::HeaterCoolerState(uint8_t Value, uint16_t AID) {
	return false;
	
	/*
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
	*/
}

bool Matter::ThresholdTemperature(float Value, uint16_t AID, bool IsCooling) {
	return false;

	/*
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
	*/
}

bool Matter::RotationSpeed(float Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return false;

	/*
	ESP_LOGE("RotationSpeed", "UUID: %04X, Value: %f", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

    	if (Iterator > 0 && IRDeviceItem.DeviceType != 0xEF) return true;

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // Air Conditionair
        	return false;

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE2, round(Value), (IRDeviceItem.Status >= 0x1000));

        return true;
    }

    return false;

	*/
}

bool Matter::TargetFanState(bool Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return false;

	/*
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
	*/
}

bool Matter::SwingMode(bool Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return true;

	/*
	ESP_LOGE("TargetFanState", "UUID: %04X, Value: %d", AID, Value);

    if (Settings.eFuse.Type == Settings.Devices.Remote) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(Converter::ToHexString(AID, 4));

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
        	return false;

        StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE3, (Value) ? 1 : 0);

        return true;
    }

    return false;
	*/
}

bool Matter::TargetPosition(uint8_t Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return false;

	/*
	ESP_LOGE("TargetPosition", "UUID: %04X, Value: %d, Iterator: %d", AID, Value, Iterator);

    if (Settings.eFuse.Type == Settings.Devices.WindowOpener) {
    	if (Iterator > 0) return true;

		hap_val_t ValueForPosition;
		ValueForPosition.u = Value;

		CommandWindowOpener_t* WindowOpener = (CommandWindowOpener_t *)Command_t::GetCommandByID(0x10);

		if (WindowOpener != nullptr) 
		{
			WindowOpener->SetPosition(Value);
			HomeKitUpdateCharValue(AID, HAP_SERV_UUID_WINDOW, HAP_CHAR_UUID_TARGET_POSITION, ValueForPosition);
			return true;
		}
	}

    return false;
	*/
}

bool Matter::GetConfiguredName(char* Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return false;

	/*
	ESP_LOGE("GetConfiguredName", "UUID: %04X, Value: %s", AID, Value);
	uint32_t IID = hap_serv_get_iid(hap_char_get_parent(Char));

	return true;
	*/
}

bool Matter::SetConfiguredName(char* Value, uint16_t AID, uint8_t *Char, uint8_t Iterator) {
	return false;

	/*
	ESP_LOGE("SetConfiguredName", "UUID: %04X, Value: %s", AID, Value);
	uint32_t IID = hap_serv_get_iid(hap_char_get_parent(Char));

	string KeyName = string(NVS_HOMEKIT_CNPREFIX) + Converter::ToHexString(AID, 4) + Converter::ToHexString(IID, 8);

	ESP_LOGE("!", "%s", KeyName.c_str());

	NVS Memory(NVS_HOMEKIT_AREA);
	Memory.SetString(Converter::ToLower(KeyName), string(Value));
	Memory.Commit();

	return true;
	*/
}

void Matter::StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value, bool Send) {
	return;

	/*

	pair<bool, uint16_t> Result = ((DataRemote_t*)Data)->StatusUpdateForDevice(UUID, FunctionID, Value, "", false, true);

	ESP_LOGE("RESULT", "StatusACUpdateIRSend %02X %02X %u Result.second %04X", FunctionID, Value, Send, Result.second);

	if (!Send) return;

	if (!Result.first) return;
	if (Codeset == 0) return;

    CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

    if (IRCommand == nullptr) return;

    string Operand = Converter::ToHexString(Codeset,4);

    if (FunctionID == 0xE3 && ACOperand::IsSwingSeparateForCodeset(Codeset))
    	Operand += ("FFF" + Converter::ToHexString((Value == 0) ? 2 : 1,1));
    else
    	Operand += Converter::ToHexString(Result.second,4);

    IRCommand->Execute(0xEF, Operand.c_str());
	*/
}

/* A dummy callback for handling a write on the "On" characteristic of Fan.
 * In an actual accessory, this should control the hardware
 */
/*
int Matter::WriteCallback(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv)
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
		else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_POSITION)) {
			TargetPosition(write->val.u, AID, write->hc, i);
            *(write->status) = HAP_STATUS_SUCCESS;
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
*/

void Matter::CreateAccessories(intptr_t context) {
    //Esp32AppServer::Init(); // Init ZCL Data Model and CHIP App Server AND Initialize device attestation config
    
	//!hap_set_debug_level(HAP_DEBUG_LEVEL_WARN);

	switch (Settings.eFuse.Type) {
		case Settings_t::Devices_t::Remote:
			return CreateRemoteBridge();
			break;
		case Settings_t::Devices_t::WindowOpener:
			return CreateWindowOpener();
			break;
	}
}

int AddDeviceEndpoint(MatterDevice * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList,
                      const Span<DataVersion> & dataVersionStorage, chip::EndpointId parentEndpointId)
{
    uint8_t index = 0;
    while (index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        if (NULL == gDevices[index])
        {
            gDevices[index] = dev;
            EmberAfStatus ret;
            while (1)
            {
                dev->SetEndpointId(gCurrentEndpointId);
                ret =
                    emberAfSetDynamicEndpoint(index, gCurrentEndpointId, ep, dataVersionStorage, deviceTypeList, parentEndpointId);
                if (ret == EMBER_ZCL_STATUS_SUCCESS)
                {
                    ChipLogProgress(DeviceLayer, "Added device %s to dynamic endpoint %d (index=%d)", dev->GetName(),
                                    gCurrentEndpointId, index);
                    return index;
                }
                else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS)
                {
                    return -1;
                }
                // Handle wrap condition
                if (++gCurrentEndpointId < gFirstDynamicEndpointId)
                {
                    gCurrentEndpointId = gFirstDynamicEndpointId;
                }
            }
        }
        index++;
    }
    ChipLogProgress(DeviceLayer, "Failed to add dynamic endpoint: No endpoints available!");
    return -1;
}

CHIP_ERROR RemoveDeviceEndpoint(MatterDevice * dev)
{
    for (uint8_t index = 0; index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; index++)
    {
        if (gDevices[index] == dev)
        {
            EndpointId ep   = emberAfClearDynamicEndpoint(index);
            gDevices[index] = NULL;
            ChipLogProgress(DeviceLayer, "Removed device %s from dynamic endpoint %d (index=%d)", dev->GetName(), ep, index);
            // Silence complaints about unused ep when progress logging
            // disabled.
            UNUSED_VAR(ep);
            return CHIP_NO_ERROR;
        }
    }
    return CHIP_ERROR_INTERNAL;
}

namespace {
void CallReportingCallback(intptr_t closure)
{
    auto path = reinterpret_cast<app::ConcreteAttributePath *>(closure);
    MatterReportingAttributeChangeCallback(*path);
    Platform::Delete(path);
}

void ScheduleReportingCallback(MatterDevice * dev, ClusterId cluster, AttributeId attribute)
{
    auto * path = Platform::New<app::ConcreteAttributePath>(dev->GetEndpointId(), cluster, attribute);
    DeviceLayer::PlatformMgr().ScheduleWork(CallReportingCallback, reinterpret_cast<intptr_t>(path));
}
} // anonymous namespace

void Matter::HandleDeviceStatusChanged(MatterDevice * dev, MatterDevice::Changed_t itemChangedMask)
{
    if (itemChangedMask & MatterDevice::kChanged_Reachable)
        ScheduleReportingCallback(dev, BridgedDeviceBasic::Id, BridgedDeviceBasic::Attributes::Reachable::Id);

    if (itemChangedMask & MatterDevice::kChanged_State)
        ScheduleReportingCallback(dev, OnOff::Id, OnOff::Attributes::OnOff::Id);

    if (itemChangedMask & MatterDevice::kChanged_Name)
        ScheduleReportingCallback(dev, BridgedDeviceBasic::Id, BridgedDeviceBasic::Attributes::NodeLabel::Id);
}

void Matter::CreateRemoteBridge() {
	ESP_LOGE(Tag, "CreateRemoteBridge");

     // Set starting endpoint id where dynamic endpoints will be assigned, which
    // will be the next consecutive endpoint id after the last fixed endpoint.
    gFirstDynamicEndpointId = static_cast<chip::EndpointId>(
        static_cast<int>(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1))) + 1);
    gCurrentEndpointId = gFirstDynamicEndpointId;

    // Disable last fixed endpoint, which is used as a placeholder for all of the
    // supported clusters so that ZAP will generated the requisite code.
    emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)), false);

    // A bridge has root node device type on EP0 and aggregate node device type (bridge) at EP1
    emberAfSetDeviceTypeList(0, Span<const EmberAfDeviceType>(gRootDeviceTypes));
    emberAfSetDeviceTypeList(1, Span<const EmberAfDeviceType>(gAggregateNodeDeviceTypes));

    // Add lights 1..3 --> will be mapped to ZCL endpoints 3, 4, 5
    AddDeviceEndpoint(&gLight1, &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                      Span<DataVersion>(gLight1DataVersions), 1);
    AddDeviceEndpoint(&gLight2, &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                      Span<DataVersion>(gLight2DataVersions), 1);
    AddDeviceEndpoint(&gLight3, &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                      Span<DataVersion>(gLight3DataVersions), 1);

    // Remove Light 2 -- Lights 1 & 3 will remain mapped to endpoints 3 & 5
    RemoveDeviceEndpoint(&gLight2);

    // Add Light 4 -- > will be mapped to ZCL endpoint 6
    AddDeviceEndpoint(&gLight4, &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                      Span<DataVersion>(gLight4DataVersions), 1);

    // Re-add Light 2 -- > will be mapped to ZCL endpoint 7
    AddDeviceEndpoint(&gLight2, &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffDeviceTypes),
                      Span<DataVersion>(gLight2DataVersions), 1);


    /*
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

		uint8_t product_data[] = {0x4D, 0x7E, 0xC5, 0x46, 0x80, 0x79, 0x26, 0x54};
		hap_acc_add_product_data(Accessory, product_data, sizeof(product_data));

		hap_acc_add_wifi_transport_service(Accessory, 0);

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
			case 0x02: // Media
			{
				hap_serv_t *ServiceTV;

				uint8_t PowerStatus = DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);
				uint8_t ModeStatus 	= DataDeviceItem_t::GetStatusByte(IRDevice.Status, 1);

				if (PowerStatus > 1) PowerStatus = 1;

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
	*/
}

void Matter::CreateWindowOpener() {

	/*
	string 		NameString 	=  "";
	uint16_t 	UUID 		= 0;
	uint16_t	Status		= 0;

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
			.cid 				= HAP_CID_WINDOW,
			.identify_routine 	= BridgeIdentify,
		};

		Accessory = hap_acc_create(&cfg);

		ACOperand Operand((uint32_t)Status);

		hap_serv_t *ServiceWindowOpener;

		uint8_t CurrentPos = 0;
		if (Sensor_t::GetSensorByID(0x90) != nullptr)
			CurrentPos = (uint8_t)Sensor_t::GetSensorByID(0x90)->GetValue("Position");

		if (CurrentPos > 100)
			CurrentPos = 100;

		ServiceWindowOpener = hap_serv_window_create(CurrentPos, CurrentPos, 2);
		hap_serv_add_char(ServiceWindowOpener, hap_char_name_create("Window Opener"));
		hap_serv_set_priv(ServiceWindowOpener, (void *)(uint32_t)UUID);
		hap_serv_set_write_cb(ServiceWindowOpener, WriteCallback);
		hap_acc_add_serv(Accessory, ServiceWindowOpener);

		uint8_t product_data[] = {0x4D, 0x7E, 0xC5, 0x46, 0x80, 0x79, 0x26, 0x54};
		hap_acc_add_product_data(Accessory, product_data, sizeof(product_data));

		hap_acc_add_wifi_transport_service(Accessory, 0);

		hap_add_accessory(Accessory);
	}
	else
	{
		hap_serv_t *ServiceWindowOpener = hap_acc_get_serv_by_uuid(ExistedAccessory, HAP_SERV_UUID_HEATER_COOLER);
		if (ServiceWindowOpener != NULL)
			hap_serv_set_priv(ServiceWindowOpener, (void *)(uint32_t)UUID);
	}

	return HAP_CID_WINDOW;
	*/
}

void Matter::Task(void *) {
	IsRunning = true;

	/*
	hap_set_debug_level(HAP_DEBUG_LEVEL_INFO);

    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_NAME;

    hap_set_config(&hap_cfg);

	hap_init(HAP_TRANSPORT_WIFI);

	hap_cid_t CID = FillAccessories();

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
	*/
    vTaskDelete(NULL);
}

void Matter::DeviceEventCallback(const ChipDeviceEvent * event, intptr_t arg)
{
    switch (event->Type)
    {
		case DeviceEventType::kInternetConnectivityChange:
			if (event->InternetConnectivityChange.IPv4 == kConnectivity_Established)
			{
				ChipLogProgress(Shell, "IPv4 Server ready...");
				chip::app::DnssdServer::Instance().StartServer();
			}
			else if (event->InternetConnectivityChange.IPv4 == kConnectivity_Lost)
			{
				ChipLogProgress(Shell, "Lost IPv4 connectivity...");
			}
			if (event->InternetConnectivityChange.IPv6 == kConnectivity_Established)
			{
				ChipLogProgress(Shell, "IPv6 Server ready...");
				chip::app::DnssdServer::Instance().StartServer();
			}
			else if (event->InternetConnectivityChange.IPv6 == kConnectivity_Lost)
			{
				ChipLogProgress(Shell, "Lost IPv6 connectivity...");
			}

			break;

		case DeviceEventType::kCHIPoBLEConnectionEstablished:
			ChipLogProgress(Shell, "CHIPoBLE connection established");
			break;

		case DeviceEventType::kCHIPoBLEConnectionClosed:
			ChipLogProgress(Shell, "CHIPoBLE disconnected");
			break;

		case DeviceEventType::kCommissioningComplete:
			ChipLogProgress(Shell, "Commissioning complete");
			break;

		case DeviceEventType::kInterfaceIpAddressChanged:
			ChipLogProgress(Shell, "DeviceEventType::kInterfaceIpAddressChanged");

			if ((event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV4_Assigned) ||
				(event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV6_Assigned))
			{
				// DNSSD server restart on any ip assignment: if link local ipv6 is configured, that
				// will not trigger a 'internet connectivity change' as there is no internet
				// connectivity. DNSSD still wants to refresh its listening interfaces to include the
				// newly selected address.
				chip::app::DnssdServer::Instance().StartServer();
			}
			break;
    }

    ChipLogProgress(Shell, "Current free heap: %u\n", static_cast<unsigned int>(heap_caps_get_free_size(MALLOC_CAP_8BIT)));
}

bool emberAfActionsClusterInstantActionCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                                const Actions::Commands::InstantAction::DecodableType & commandData)
{
    // No actions are implemented, just return status NotFound.
    commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::NotFound);
    return true;
}