#include "Matter.h"
#include "GenericDevice.h"
#include "Thermostat.h"

#include "TempSensor.h"

#include "Data.h"
#include "DataRemote.h"

#define Tag "MatterThermostat"

extern DataEndpoint_t *Data;

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

MatterThermostat::MatterThermostat(string szDeviceName, string szLocation) : MatterGenericDevice(szDeviceName, szLocation)
{
    DeviceType = DeviceTypeEnum::Thermostat;

    SetReachable(true);
}

int16_t MatterThermostat::GetLocalTemperature() { 
    return mLocalTempMeasurement;
} 

void MatterThermostat::SetLocalTemperature (float Value) {
    int16_t NormalizedValue = round(Value * 100);

    // Limit measurement based on the min and max.
    if (NormalizedValue < mLocalTempMin)
        NormalizedValue = mLocalTempMin;
    else if (NormalizedValue > mLocalTempMax)
        NormalizedValue = mLocalTempMax;

    bool changed = mLocalTempMeasurement != NormalizedValue;

    mLocalTempMeasurement = NormalizedValue;

    if (changed)
        HandleStatusChanged(this, kChanged_MeasurementValue);
}

chip::app::Clusters::Thermostat::ThermostatSystemMode MatterThermostat::GetMode() {
    return (chip::app::Clusters::Thermostat::ThermostatSystemMode)CurrentMode;
}

void MatterThermostat::SetMode(chip::app::Clusters::Thermostat::ThermostatSystemMode ModeToSet) {
    CurrentMode = (uint8_t)ModeToSet;
    //! Проорать о значении в HomeKit
}

float MatterThermostat::GetACTemperature() {
    return ACTemp;
}

void MatterThermostat::SetACTemperature(float Value) {
    int16_t NormalizedValue = round(Value * 100);

    bool changed = ACTemp != NormalizedValue;

    ACTemp = NormalizedValue;

    if (changed)
        HandleStatusChanged(this, kChanged_MeasurementValue);

}

void MatterThermostat::HandleStatusChanged(MatterThermostat * dev, MatterThermostat::Changed_t itemChangedMask)
{
    ESP_LOGE("MatterThermostat", "MatterThermostat");

    if (itemChangedMask & (MatterTempSensor::kChanged_Reachable | MatterTempSensor::kChanged_Name | MatterTempSensor::kChanged_Location))
        HandleDeviceStatusChanged(static_cast<MatterGenericDevice *>(dev), (MatterGenericDevice::Changed_t) itemChangedMask);

    if (itemChangedMask & MatterTempSensor::kChanged_MeasurementValue)
        ScheduleReportingCallback(dev, chip::app::Clusters::Thermostat::Id, chip::app::Clusters::Thermostat::Attributes::LocalTemperature::Id);
}

EmberAfStatus MatterThermostat::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId attributeId, uint8_t * Buffer, uint16_t maxReadLength)
{
    ESP_LOGE("MatterThermostat", "HandleReadAttribute, ClusterID: %lu, attributeID: 0x%lx, MaxReadLength: %d", ClusterID, attributeId, maxReadLength);

    if (ClusterID == chip::app::Clusters::Thermostat::Id)
    {
        ESP_LOGE("THERMOSTAT", "handler");
        if ((attributeId == Thermostat::Attributes::LocalTemperature::Id) && (maxReadLength == 2))
        {
            int16_t measuredValue = GetLocalTemperature();
            memcpy(Buffer, &measuredValue, sizeof(measuredValue));
        }
        else if ((attributeId == Thermostat::Attributes::SystemMode::Id) && (maxReadLength == 1))
        {
            *Buffer = CurrentMode;
        }
        else if ((attributeId == Thermostat::Attributes::ThermostatRunningMode::Id) && (maxReadLength == 1))
        {
            *Buffer = CurrentMode;
        }
        else if ((     attributeId == Thermostat::Attributes::MinCoolSetpointLimit::Id 
                    || attributeId == Thermostat::Attributes::MinHeatSetpointLimit::Id
                    || attributeId == Thermostat::Attributes::AbsMinHeatSetpointLimit::Id
                    || attributeId == Thermostat::Attributes::AbsMinCoolSetpointLimit::Id) 
                    && (maxReadLength == 2))
        {
            int16_t MinValue = 1600;
            memcpy(Buffer, &MinValue, sizeof(MinValue));
        }
        else if ((     attributeId == Thermostat::Attributes::MaxCoolSetpointLimit::Id 
                    || attributeId == Thermostat::Attributes::MaxHeatSetpointLimit::Id
                    || attributeId == Thermostat::Attributes::AbsMaxHeatSetpointLimit::Id
                    || attributeId == Thermostat::Attributes::AbsMaxCoolSetpointLimit::Id) 
                    && (maxReadLength == 2))                {
            int16_t MaxValue = 3000;
            memcpy(Buffer, &MaxValue, sizeof(MaxValue));
        }
        else if ((attributeId == Thermostat::Attributes::OccupiedCoolingSetpoint::Id || attributeId == Thermostat::Attributes::OccupiedHeatingSetpoint::Id) && (maxReadLength == 2))
        {
            int16_t CurValue = ACTemp;
            memcpy(Buffer, &CurValue, sizeof(CurValue));
        }
        else if ((attributeId == Thermostat::Attributes::FeatureMap::Id) && (maxReadLength == 4))
        {
            uint32_t FeatureMap = ZCL_THERMOSTAT_FEATURE_MAP;
            memcpy(Buffer, &FeatureMap, sizeof(FeatureMap));
        }
        else if ((attributeId == ClusterRevision::Id) && (maxReadLength == 2))
        {
            uint16_t ClusterRevision = ZCL_THERMOSTAT_CLUSTER_REVISION;
            memcpy(Buffer, &ClusterRevision, sizeof(ClusterRevision));
        }
        else
        {
            return EMBER_ZCL_STATUS_FAILURE; 
        }
    }
    
    if (ClusterID == chip::app::Clusters::FanControl::Id) // Fan Control
    {
        ESP_LOGE("FAN", "handler");

        if ((attributeId == FanControl::Attributes::FanMode::Id) && (maxReadLength == 1))
        {
            uint8_t CurrentFanMode = 5; // Auto                
            *Buffer = CurrentFanMode;
        }
        else if ((attributeId == Thermostat::Attributes::ControlSequenceOfOperation::Id) && (maxReadLength == 1)) 
        {
            uint8_t ControlSequence = 0x4;
            *Buffer = ControlSequence;
        }
        else if ((attributeId == FanControl::Attributes::PercentSetting::Id || attributeId == FanControl::Attributes::PercentCurrent::Id) && (maxReadLength == 1)) 
        {
            uint8_t CurrentValue = 0x1;
            *Buffer = CurrentValue;
        }
        else if ((attributeId == FanControl::Attributes::FeatureMap::Id) && (maxReadLength == 4))
        {
            uint32_t FanFeatureMap = ZCL_FANCONTROL_FEATURE_MAP;
            memcpy(Buffer, &FanFeatureMap, sizeof(FanFeatureMap));
        }
        else if ((attributeId == ClusterRevision::Id) && (maxReadLength == 2))
        {
            uint16_t ClusterRevision = ZCL_FANCONTROL_CLUSTER_REVISION;
            memcpy(Buffer, &ClusterRevision, sizeof(ClusterRevision));
        }
        else if (((attributeId == FanControl::Attributes::RockSupport::Id) || (attributeId == FanControl::Attributes::RockSetting::Id)) && (maxReadLength == 1))
        {
            uint8_t RockSettings = 0x2;  // Rock setting             
            *Buffer = RockSettings;
        }
        else if ((attributeId == FanControl::Attributes::FanModeSequence::Id) && (maxReadLength == 1))
        {
            uint8_t FanModeSequence = 2; // Off/Low/Med/High/Auto         
            *Buffer = FanModeSequence;
        }
        else if ((attributeId == FanControl::Attributes::SpeedSetting::Id) && (maxReadLength == 1))
        {
            uint8_t SpeedSetting = 0x0;  // Vertical Swing              
            *Buffer = SpeedSetting;
        }
        else if ((attributeId == FanControl::Attributes::PercentCurrent::Id) && (maxReadLength == 1))
        {
            uint8_t SpeedSetting = 0x4;             
            *Buffer = SpeedSetting;
        }                
        else
        {
            return EMBER_ZCL_STATUS_FAILURE; 
        }
    }

    ESP_LOGE("Thermostat", "EMBER_ZCL_STATUS_SUCCESS");
    return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus MatterThermostat::HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value)
{
    ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Thermostat: clusterID=%lu attrId=0x%lx", ClusterID, AttributeID);

    //CurrentMode = *Value;

    if (ClusterID == 0x0201)
    {
        ThermostatClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0402)
    {
        TemperatureMeasurementClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x001C)
    {
        ThermostatUIConfigClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0202)
    {
        FanControlClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0200)
    {
        ThermostatOperatingStateClusterHandler(AttributeID, Value);
    }
    else
    {
        ESP_LOGE(Tag, "Wrong ClusterID: %lu", ClusterID);
    }
    bool Result = false;

    if (ClusterID == 0x201 && AttributeID == 0x1C)
        Result = HandleModeChange(*Value);

    return Result ? EMBER_ZCL_STATUS_SUCCESS : EMBER_ZCL_STATUS_FAILURE;
}

void MatterThermostat::HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask)
{
    ESP_LOGE("MatterThermostat", "HandleDeviceChange");

    HandleStatusChanged(this, (MatterThermostat::Changed_t) changeMask);
}

bool MatterThermostat::HandleModeChange(uint8_t Value) {
    // Mode == 0 - Выключено
    // Mode == 1 - АВТОМАТИЧЕСКИ
    // Mode == 3 - Охлаждение
    // Mode == 4 - НАГРЕВ

    if (Settings.eFuse.Type == Settings.Devices.Remote && IsBridgedDevice) {
        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(BridgedUUID);

        if (IRDeviceItem.IsEmpty() || IRDeviceItem.DeviceType != 0xEF) // air conditionair
            return false;

        CurrentMode = Value;

        uint8_t ACOperandValue = Value;

        switch (Value)
        {
            case 3: ACOperandValue = 2; break;
            case 4: ACOperandValue = 3; break;
            default: break;
        }

        if (ACOperandValue > 0) 
        {
            /*
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
            */
        }

        Matter::StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, ACOperandValue);
        return true;
    }

    return false;
}

void MatterThermostat::ThermostatClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "ThermostatClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //Local Temperature
    {

    }
    else if(AttributeID == 0x000B)  //Abs Min Heat
    {

    }
    else if(AttributeID == 0x000C)  //Abs Max Heat
    {

    }
    else if(AttributeID == 0x000D)  //Abs Min Cool
    {

    }
    else if(AttributeID == 0x000E)  //Abs Max Cool
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterThermostat::TemperatureMeasurementClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "TemperatureMeasurementClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //Measured Value
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterThermostat::ThermostatUIConfigClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "ThermostatUIConfigClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //Display Temperature
    {

    }
    else if(AttributeID == 0x0008)   //Keypad Lockout
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterThermostat::FanControlClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "FanControlClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //Fan Mode
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterThermostat::ThermostatOperatingStateClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "ThermostatOperatingStateClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //Thermostat State
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}
