#include "Matter.h"


#include "Globals.h"

#include "GenericDevice.h"
#include "DeviceCallbacks.h"
#include "NetworkCommissioningCustomDriver.h"
#include "MatterWiFi.h"
#include "DataRemote.h"

#include "SensorMeteo.h"
#include "CommandIR.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/ConcreteAttributePath.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
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

#include "HumiditySensor.h"
#include "Light.h"
#include "Outlet.h"
#include "TempSensor.h"
#include "Thermostat.h"
#include "VideoPlayer.h"

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

const char *Tag = "Matter";

TaskHandle_t 		Matter::TaskHandle = NULL;

bool 				Matter::IsRunning 		= false;
bool 				Matter::IsAP 			= false;

uint64_t			Matter::TVHIDLastUpdated = 0;

vector<uint8_t*> 	Matter::BridgedAccessories = vector<uint8_t*>();

using namespace ::chip;
using namespace ::chip::DeviceManager;
using namespace ::chip::DeviceLayer;
using namespace ::chip::Platform;
using namespace ::chip::Credentials;
using namespace ::chip::app::Clusters;

static AppDeviceCallbacks AppCallback;
//static AppDeviceCallbacksDelegate sAppDeviceCallbacksDelegate;

static EndpointId gCurrentEndpointId;
static EndpointId gFirstDynamicEndpointId;

#define DEVICE_TYPE_BRIDGED_NODE 			0x0013
#define DEVICE_TYPE_ROOT_NODE 				0x0016
#define DEVICE_TYPE_BRIDGE 					0x000E
#define DEVICE_REVISION_DEFAULT 			1

#define DEVICE_TYPE_LO_ON_OFF_LIGHT 		0x0100
#define DEVICE_REVISION_LO_ON_OFF_LIGHT		2

#define DEVICE_TYPE_ON_OFF_OUTLET 			0x010A
#define DEVICE_REVISION_ON_OFF_OUTLET 		2

#define DEVICE_TYPE_TEMP_SENSOR 			0x0302
#define DEVICE_REVISION_TEMP_SENSOR			2

#define DEVICE_TYPE_HUMIDITY_SENSOR 		0x0307
#define DEVICE_REVISION_HUMIDITY_SENSOR		2

#define DEVICE_TYPE_THERMOSTAT 				0x0301
#define DEVICE_REVISION_THERMOSTAT			2

#define DEVICE_TYPE_FANCONTROL 				0x002B
#define DEVICE_REVISION_FANCONTROL 			1

#define DEVICE_TYPE_VIDEOPLAYER 			0x0028 
#define DEVICE_REVISION_VIDEOPLAYER 		1

#define kNodeLabelSize 						2
#define kDescriptorAttributeArraySize		254

/* REVISION definitions:
 */

#define ZCL_DESCRIPTOR_CLUSTER_REVISION 			(1u)
#define ZCL_BRIDGED_DEVICE_BASIC_CLUSTER_REVISION 	(1u)
#define ZCL_FIXED_LABEL_CLUSTER_REVISION 			(1u)

const EmberAfDeviceType gRootDeviceTypes[] 					= { { DEVICE_TYPE_ROOT_NODE, DEVICE_REVISION_DEFAULT } };
const EmberAfDeviceType gAggregateNodeDeviceTypes[] 		= { { DEVICE_TYPE_BRIDGE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedOnOffLightDeviceTypes[] 	= { { DEVICE_TYPE_LO_ON_OFF_LIGHT, DEVICE_REVISION_LO_ON_OFF_LIGHT },
                                                       			{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedOnOffOutletDeviceTypes[] 	= { { DEVICE_TYPE_ON_OFF_OUTLET, DEVICE_REVISION_ON_OFF_OUTLET },
                                                       			{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedTempSensorDeviceTypes[] 	= { { DEVICE_TYPE_TEMP_SENSOR, DEVICE_REVISION_TEMP_SENSOR },
                                                            	{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedHumiditySensorDeviceTypes[] = { { DEVICE_TYPE_HUMIDITY_SENSOR, DEVICE_REVISION_HUMIDITY_SENSOR },
                                                            	{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedThermostatDeviceTypes[] 	= { { DEVICE_TYPE_THERMOSTAT, DEVICE_REVISION_THERMOSTAT },
                                                            	{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedFanDeviceTypes[] 			= { { DEVICE_TYPE_FANCONTROL, DEVICE_REVISION_FANCONTROL },
                                                            	{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

const EmberAfDeviceType gBridgedVideoPLayer[] 				= { { DEVICE_TYPE_VIDEOPLAYER, DEVICE_REVISION_VIDEOPLAYER },
                                                            	{ DEVICE_TYPE_BRIDGED_NODE, DEVICE_REVISION_DEFAULT } };

// Declare Descriptor cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(descriptorAttrs)
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::DeviceTypeList::Id	, ARRAY, kDescriptorAttributeArraySize, 0),		/* device list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::ServerList::Id		, ARRAY, kDescriptorAttributeArraySize, 0), 	/* server list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::ClientList::Id		, ARRAY, kDescriptorAttributeArraySize, 0), 	/* client list */
    DECLARE_DYNAMIC_ATTRIBUTE(Descriptor::Attributes::PartsList::Id			, ARRAY, kDescriptorAttributeArraySize, 0),  	/* parts list */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// Declare Bridged Device Basic information cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(bridgedDeviceBasicAttrs)
    DECLARE_DYNAMIC_ATTRIBUTE(BridgedDeviceBasicInformation::Attributes::NodeLabel::Id, CHAR_STRING, kNodeLabelSize, 0), /* NodeLabel */
    DECLARE_DYNAMIC_ATTRIBUTE(BridgedDeviceBasicInformation::Attributes::Reachable::Id, BOOLEAN, 1, 0),               /* Reachable */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

// ---------------------------------------------------------------------------
//
// LIGHT ENDPOINT: contains the following clusters:
//   - On/Off
//   - Descriptor
//   - Bridged Device Basic

// Declare Cluster List for Bridged Light endpoint
// TODO: It's not clear whether it would be better to get the command lists from
// the ZAP config on our last fixed endpoint instead.
constexpr chip::CommandId onOffIncomingCommands[] = {
    chip::app::Clusters::OnOff::Commands::Off::Id,
    chip::app::Clusters::OnOff::Commands::On::Id,
    chip::app::Clusters::OnOff::Commands::Toggle::Id,
    chip::app::Clusters::OnOff::Commands::OffWithEffect::Id,
    chip::app::Clusters::OnOff::Commands::OnWithRecallGlobalScene::Id,
    chip::app::Clusters::OnOff::Commands::OnWithTimedOff::Id,
    chip::kInvalidCommandId,
};

// Declare On/Off cluster attributes
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(onOffAttrs)
    DECLARE_DYNAMIC_ATTRIBUTE(OnOff::Attributes::OnOff::Id					, BOOLEAN, 	1, 0), /* on/off */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedLightClusters)
    DECLARE_DYNAMIC_CLUSTER(OnOff::Id							, onOffAttrs, onOffIncomingCommands, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs, nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Light endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedLightEndpoint, bridgedLightClusters);

// ---------------------------------------------------------------------------
//
// OUTLET ENDPOINT: contains the following clusters:
//   - On/Off
//   - Descriptor
//   - Bridged Device Basic

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedOutletClusters)
    DECLARE_DYNAMIC_CLUSTER(OnOff::Id							, onOffAttrs, onOffIncomingCommands, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs, nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Light endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedOutletEndpoint, bridgedOutletClusters);


// ---------------------------------------------------------------------------
//
// TEMPERATURE SENSOR ENDPOINT: contains the following clusters:
//   - Temperature measurement
//   - Descriptor
//   - Bridged Device Basic

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(tempSensorAttrs)
	DECLARE_DYNAMIC_ATTRIBUTE(TemperatureMeasurement::Attributes::MeasuredValue::Id		, INT16S	, 2, 0), /* Measured Value */
    DECLARE_DYNAMIC_ATTRIBUTE(TemperatureMeasurement::Attributes::MinMeasuredValue::Id	, INT16S	, 2, 0), /* Min Measured Value */
    DECLARE_DYNAMIC_ATTRIBUTE(TemperatureMeasurement::Attributes::MaxMeasuredValue::Id	, INT16S	, 2, 0), /* Max Measured Value */
    DECLARE_DYNAMIC_ATTRIBUTE(TemperatureMeasurement::Attributes::FeatureMap::Id		, BITMAP32	, 4, 0), /* FeatureMap */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedTempSensorClusters)
	DECLARE_DYNAMIC_CLUSTER(TemperatureMeasurement::Id			, tempSensorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs, nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Temp sensor endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedTempSensorEndpoint, bridgedTempSensorClusters);

// ---------------------------------------------------------------------------
//
// RELATIVE HUMIDITY SENSOR ENDPOINT: contains the following clusters:
//   - Relative humidity measurement
//   - Descriptor
//   - Bridged Device Basic

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(humiditySensorAttrs)
	DECLARE_DYNAMIC_ATTRIBUTE(RelativeHumidityMeasurement::Attributes::MeasuredValue::Id	, INT16U	, 2, 0), /* Measured Value */
    DECLARE_DYNAMIC_ATTRIBUTE(RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Id	, INT16U	, 2, 0), /* Min Measured Value */
    DECLARE_DYNAMIC_ATTRIBUTE(RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Id	, INT16U	, 2, 0), /* Max Measured Value */
    DECLARE_DYNAMIC_ATTRIBUTE(RelativeHumidityMeasurement::Attributes::FeatureMap::Id		, BITMAP32	, 4, 0), /* FeatureMap */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedHumiditySensorClusters)
	DECLARE_DYNAMIC_CLUSTER(RelativeHumidityMeasurement::Id		, humiditySensorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs, nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Temp sensor endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedHumiditySensorEndpoint, bridgedHumiditySensorClusters);

// ---------------------------------------------------------------------------
//
// THERMOSTAT ENDPOINT: contains the following clusters:
//   - Thermostat
//   - FanControl
//   - Descriptor
//   - Bridged Device Basic

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(thermostatAttrs)
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::LocalTemperature::Id			, INT16S, 	2, ZAP_ATTRIBUTE_MASK(NULLABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::OccupiedCoolingSetpoint::Id	, INT16S, 	2, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::OccupiedHeatingSetpoint::Id	, INT16S,	2, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::MinHeatSetpointLimit::Id		, INT16S, 	2, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::MaxHeatSetpointLimit::Id		, INT16S, 	2, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::MinCoolSetpointLimit::Id		, INT16S, 	2, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::MaxCoolSetpointLimit::Id		, INT16S, 	2, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::AbsMinHeatSetpointLimit::Id	, INT16S, 	2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::AbsMaxHeatSetpointLimit::Id	, INT16S, 	2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::AbsMinCoolSetpointLimit::Id	, INT16S, 	2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::AbsMaxCoolSetpointLimit::Id	, INT16S, 	2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::MinSetpointDeadBand::Id		, INT8S, 	1, 	ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::ControlSequenceOfOperation::Id, ENUM8, 	1, 	ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::SystemMode::Id				, ENUM8, 	1,	ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Thermostat::Attributes::FeatureMap::Id				, BITMAP32, 4, 0),
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();Thermostat

constexpr chip::CommandId ThermostatIncomingCommands[] = {
	chip::app::Clusters::Thermostat::Commands::SetpointRaiseLower::Id,
	chip::kInvalidCommandId,
};

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedThermostatClusters)
	DECLARE_DYNAMIC_CLUSTER(Thermostat::Id						, thermostatAttrs, ThermostatIncomingCommands, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs, nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Thermostat endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedThermostatEndpoint, bridgedThermostatClusters);

// ---------------------------------------------------------------------------
//
// Fan control ENDPOINT: contains the following clusters:
//   - FanControl
//   - Descriptor
//   - Bridged Device Basic

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(fanControlAttrs)
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::FanMode::Id					, ENUM8, 	1, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::FanModeSequence::Id			, ENUM8, 	1, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::PercentSetting::Id			, INT8U, 	1, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE) | ZAP_ATTRIBUTE_MASK(NULLABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::PercentCurrent::Id			, INT8U, 	1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::SpeedSetting::Id				, INT8U, 	1, ZAP_ATTRIBUTE_MASK(MIN_MAX) | ZAP_ATTRIBUTE_MASK(WRITABLE) | ZAP_ATTRIBUTE_MASK(NULLABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::RockSupport::Id				, BITMAP8, 	1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::RockSetting::Id				, BITMAP8, 	1, ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(FanControl::Attributes::FeatureMap::Id				, BITMAP32, 4, 0),
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedFanClusters)
	DECLARE_DYNAMIC_CLUSTER(FanControl::Id						, fanControlAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs, nullptr, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs, nullptr, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Thermostat endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedFanEndpoint, bridgedFanClusters);

// ---------------------------------------------------------------------------
//
// Basic video player ENDPOINT: contains the following clusters:
//   - OnOff
//   - Keypad Input
//   - Descriptor
//   - Bridged Device Basic

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(KeypadInputAttrs)
	DECLARE_DYNAMIC_ATTRIBUTE(KeypadInput::Attributes::FeatureMap::Id			, BITMAP32, 4, 0),
DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();


constexpr chip::CommandId KeypadInputIncomingCommands[] = {
	KeypadInput::Commands::SendKey::Id,
	KeypadInput::Commands::SendKeyResponse::Id,
	chip::kInvalidCommandId,
};

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedVideoPlayerClusters)
    DECLARE_DYNAMIC_CLUSTER(OnOff::Id							, onOffAttrs				, onOffIncomingCommands			, nullptr),
    DECLARE_DYNAMIC_CLUSTER(KeypadInput::Id						, KeypadInputAttrs			, KeypadInputIncomingCommands	, nullptr),
    DECLARE_DYNAMIC_CLUSTER(Descriptor::Id						, descriptorAttrs			, nullptr						, nullptr),
    DECLARE_DYNAMIC_CLUSTER(BridgedDeviceBasicInformation::Id	, bridgedDeviceBasicAttrs	, nullptr						, nullptr)
DECLARE_DYNAMIC_CLUSTER_LIST_END;

// Declare Bridged Thermostat endpoint
DECLARE_DYNAMIC_ENDPOINT(bridgedVideoPlayerEndpoint, bridgedVideoPlayerClusters);


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

/* Ember Handlers */

EmberAfStatus HandleReadBridgedDeviceBasicAttribute(MatterGenericDevice * dev, chip::AttributeId attributeId, uint8_t * buffer,
                                                    uint16_t maxReadLength)
{
    ChipLogProgress(DeviceLayer, "HandleReadBridgedDeviceBasicAttribute: attrId=%08lX, maxReadLength=%d", attributeId, maxReadLength);

    if ((attributeId == BridgedDeviceBasicInformation::Attributes::Reachable::Id) && (maxReadLength == 1))
    {
        *buffer = dev->IsReachable() ? 1 : 0;
    }
    else if ((attributeId == BridgedDeviceBasicInformation::Attributes::NodeLabel::Id) && (maxReadLength == 32))
    {
        MutableByteSpan zclNameSpan(buffer, maxReadLength);
        MakeZclCharString(zclNameSpan, dev->GetName());
    }
    else if ((attributeId == Globals::Attributes::ClusterRevision::Id) && (maxReadLength == 2))
    {
        *buffer = (uint16_t) ZCL_BRIDGED_DEVICE_BASIC_CLUSTER_REVISION;
    }
    else
    {
        return EMBER_ZCL_STATUS_FAILURE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                   const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer,
                                                   uint16_t maxReadLength)
{
	ESP_LOGE(Tag, "emberAfExternalAttributeReadCallback, EndpointID %d, ClusterID 0x%0lX", endpoint, clusterId);

    ///////////////////////////

    uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

    MatterGenericDevice* FindedDevice = Matter::GetDeviceByDynamicIndex(endpointIndex);

    if (FindedDevice != nullptr)
    {
        if (clusterId == BridgedDeviceBasicInformation::Id)
        {
            return HandleReadBridgedDeviceBasicAttribute(FindedDevice, attributeMetadata->attributeId, buffer, maxReadLength);
        }

		return FindedDevice->HandleReadAttribute(clusterId, attributeMetadata->attributeId, buffer, maxReadLength);
    }

    return EMBER_ZCL_STATUS_FAILURE;
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId ClusterID, const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer)
{
	ESP_LOGE(Tag, "emberAfExternalAttributeWriteCallback, EndpointID %d, ClusterID %04lX", endpoint, ClusterID);

    uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	MatterGenericDevice * dev = Matter::GetDeviceByDynamicIndex(endpointIndex);

	if (dev != nullptr)
		if ((dev->IsReachable()))
			return dev->HandleWriteAttribute(ClusterID, attributeMetadata->attributeId, buffer);

    return EMBER_ZCL_STATUS_FAILURE;
}

bool emberAfActionsClusterInstantActionCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                                const Actions::Commands::InstantAction::DecodableType & commandData)
{
    // No actions are implemented, just return status NotFound.
	ESP_LOGE(Tag, "emberAfActionsClusterInstantActionCallback");

    commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::NotFound);
    return true;
}

void Matter::WiFiSetMode(bool sIsAP, string sSSID, string sPassword) {
	IsAP 		= sIsAP;
}

void Matter::Init() {
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
}

app::Clusters::NetworkCommissioning::Instance
    sWiFiNetworkCommissioningInstance(0 /* Endpoint Id */, &(chip::DeviceLayer::NetworkCommissioning::ESPCustomWiFiDriver::GetInstance()));

void Matter::StartServer() {
	//esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, PlatformManagerImpl::HandleESPSystemEvent, NULL);
	//esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, PlatformManagerImpl::HandleESPSystemEvent, NULL);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, PlatformManagerImpl::HandleESPSystemEvent, NULL);

    chip::DeviceLayer::PlatformMgr().ScheduleWork(StartServerInner, reinterpret_cast<intptr_t>(nullptr));
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
	return Device.IsMatterEnabled();
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


void Matter::SendIRWrapper(string UUID, string params)
{
    string operand = UUID + params;

	ESP_LOGE("OPERAND", "%s", operand.c_str());

    CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");
    IRCommand->Execute(0xFE, operand.c_str());
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

void Matter::StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value, bool Send) {
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

	switch (Settings.eFuse.Type) {
		case Settings_t::Devices_t::Remote:
			return CreateRemoteBridge();
			break;
		case Settings_t::Devices_t::WindowOpener:
			return CreateWindowOpener();
			break;
	}
}


int Matter::AddDeviceEndpoint(MatterGenericDevice * dev, EmberAfEndpointType * ep, const Span<const EmberAfDeviceType> & deviceTypeList, chip::EndpointId parentEndpointId)
{
    uint8_t index = 0;

	vector<chip::EndpointId> ExistedIndexes = vector<chip::EndpointId>();
	for (auto& MatterDeviceItem : MatterDevices)
		ExistedIndexes.push_back(MatterDeviceItem->DynamicIndex);
 
    while (index < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        if (std::find(ExistedIndexes.begin(), ExistedIndexes.end(), index) == ExistedIndexes.end())
        {
			dev->DynamicIndex = index;
            MatterDevices.push_back(dev);

            EmberAfStatus ret;
            while (1)
            {
				MatterDevices.back()->SetEndpointID(gCurrentEndpointId);
                
				ret = emberAfSetDynamicEndpoint(index, gCurrentEndpointId, ep, Span<DataVersion>(dev->dataVersions), deviceTypeList, parentEndpointId);

				if (ret == EMBER_ZCL_STATUS_SUCCESS)
                {
                    ChipLogProgress(DeviceLayer, "Added device %s to dynamic endpoint %d (index=%d)", dev->GetName(),
                                    gCurrentEndpointId, index);
                    return index;
                }
                else if (ret != EMBER_ZCL_STATUS_DUPLICATE_EXISTS)
                {
					ESP_LOGE("!", "!= EMBER_ZCL_STATUS_DUPLICATE_EXISTS");
                    return -1;
                }

                // Handle wrap condition
                if (++gCurrentEndpointId < gFirstDynamicEndpointId)
                    gCurrentEndpointId = gFirstDynamicEndpointId;
            }
        }
        index++;
    }
    ChipLogProgress(DeviceLayer, "Failed to add dynamic endpoint: No endpoints available!");
    return -1;
}

CHIP_ERROR Matter::RemoveDeviceEndpoint(MatterGenericDevice * dev)
{
    for (uint8_t i = 0; i < MatterDevices.size(); i++)
    {
        if (MatterDevices[i] == dev)
        {
			uint8_t Index = MatterDevices[i]->DynamicIndex;
            EndpointId ep = emberAfClearDynamicEndpoint(Index);

            MatterDevices.erase(MatterDevices.begin()+i);

            ChipLogProgress(DeviceLayer, "Removed device %s from dynamic endpoint %d (index=%d)", dev->GetName(), ep, Index);
            // Silence complaints about unused ep when progress logging
            // disabled.
            UNUSED_VAR(ep);
            return CHIP_NO_ERROR;
        }
    }
    return CHIP_ERROR_INTERNAL;
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

	
	MatterDevices.clear();
	
	for (auto &IRCachedDevice : ((DataRemote_t *)Data)->IRDevicesCache)
	{
		DataRemote_t::IRDevice IRDevice = ((DataRemote_t *)Data)->GetDevice(IRCachedDevice.DeviceID);

		static string Name = "";
		Name = IRDevice.Name;

		switch (IRDevice.Type) 
		{
			case 0x01: // TV
			case 0x02: // Media
			{
				MatterGenericDevice* VideoPlayerToAdd = new MatterVideoPlayer(IRDevice.Name);
				VideoPlayerToAdd->IsBridgedDevice = true;
				VideoPlayerToAdd->BridgedUUID = IRDevice.UUID;
				AddDeviceEndpoint(VideoPlayerToAdd, &bridgedVideoPlayerEndpoint, Span<const EmberAfDeviceType>(gBridgedVideoPLayer), 1);
				break;
			}
			case 0x03: // light
			{
				MatterLight *LightToAdd = new MatterLight(IRDevice.Name);
				LightToAdd->IsBridgedDevice = true;
				LightToAdd->BridgedUUID = IRDevice.UUID;
				AddDeviceEndpoint(LightToAdd, &bridgedLightEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffLightDeviceTypes), 1);
				break;
			}
			case 0x05: 
			{ // Purifier
			/*
				uint8_t PowerStatus 	= DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);
				if (PowerStatus > 0) PowerStatus = 1;

				hap_serv_t *ServicePurifier;
				ServicePurifier = hap_serv_air_purifier_create(PowerStatus, (PowerStatus > 0) ? 2 : 0 , 0);
				hap_serv_add_char(ServicePurifier, hap_char_name_create(accessory_name));

				hap_serv_set_priv(ServicePurifier, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServicePurifier, WriteCallback);
				hap_acc_add_serv(Accessory, ServicePurifier);
				*/

				break;
			}
			case 0x06: // Vacuum cleaner
			{
                MatterOutlet* OutletToAdd = new MatterOutlet(IRDevice.Name);
				OutletToAdd->IsBridgedDevice = true;
				OutletToAdd->BridgedUUID = IRDevice.UUID;

                AddDeviceEndpoint(OutletToAdd, &bridgedOutletEndpoint, Span<const EmberAfDeviceType>(gBridgedOnOffOutletDeviceTypes), 1);
				break;
			}
			case 0x07: // Fan
			{
				/*
				uint8_t PowerStatus 	= DataDeviceItem_t::GetStatusByte(IRDevice.Status, 0);

				hap_serv_t *ServiceFan;
				ServiceFan = hap_serv_fan_v2_create(PowerStatus);
				hap_serv_add_char(ServiceFan, hap_char_name_create(accessory_name));

				hap_serv_set_priv(ServiceFan, (void *)(uint32_t)AID);
				hap_serv_set_write_cb(ServiceFan, WriteCallback);
				hap_acc_add_serv(Accessory, ServiceFan);
				break;
				*/
			}
			case 0xEF: // AC
			{
				MatterGenericDevice* ThermostatToAdd = new MatterThermostat(IRDevice.Name);
				
				ThermostatToAdd->IsBridgedDevice = true;
				ThermostatToAdd->BridgedUUID = IRDevice.UUID;

				AddDeviceEndpoint(ThermostatToAdd, &bridgedThermostatEndpoint, Span<const EmberAfDeviceType>(gBridgedThermostatDeviceTypes), 1);

				break;
			}
			default:
			break;
		}
	}

	SensorMeteo_t *Meteo = (SensorMeteo_t *)Sensor_t::GetSensorByID(0xFE);

	if (Meteo != nullptr && Meteo->GetSIEFlag()) 
	{
		SensorMeteo_t::SensorData MeteoSensorData = Meteo->GetValueFromSensor();

		MatterGenericDevice* TempSensorToAdd = new MatterTempSensor("Temperature sensor");
		TempSensorToAdd->IsBridgedDevice = true;
		TempSensorToAdd->BridgedUUID = "FFFE";
		((MatterTempSensor *)TempSensorToAdd)->SetTemperature(MeteoSensorData.Temperature, false);
		AddDeviceEndpoint(TempSensorToAdd, &bridgedTempSensorEndpoint, Span<const EmberAfDeviceType>(gBridgedTempSensorDeviceTypes), 1);

		MatterGenericDevice* HumiditySensorToAdd = new MatterHumiditySensor("Humidity sensor");
		HumiditySensorToAdd->IsBridgedDevice = true;
		HumiditySensorToAdd->BridgedUUID = "FFFF";
		((MatterHumiditySensor *)HumiditySensorToAdd)->SetHumidity(MeteoSensorData.Humidity, false);
		AddDeviceEndpoint(HumiditySensorToAdd, &bridgedHumiditySensorEndpoint, Span<const EmberAfDeviceType>(gBridgedHumiditySensorDeviceTypes), 1);
	}
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

MatterGenericDevice* Matter::GetDeviceByDynamicIndex(uint16_t Index) {
    for (auto& DeviceItem : MatterDevices)
        if (DeviceItem != nullptr && DeviceItem->DynamicIndex == Index)
            return DeviceItem;

    return nullptr;
}

MatterGenericDevice* Matter::GetBridgedAccessoryByType(MatterGenericDevice::DeviceTypeEnum Type, string UUID) {
    for (auto& DeviceItem : MatterDevices)
		if (DeviceItem->GetTypeName() == Type) {
			if (UUID == "" || (UUID != "" && Converter::ToUpper(DeviceItem->BridgedUUID) == Converter::ToUpper(UUID)))
				return DeviceItem;
		}

	return nullptr;
}
