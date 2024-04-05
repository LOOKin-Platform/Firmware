#include "Matter.h"
#include "GenericDevice.h"
#include "Thermostat.h"

#include "TempSensor.h"

#include "DataRemote.h"
#include "CommandIR.h"

#define Tag "MatterThermostat"

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;
using namespace Protocols::InteractionModel;


MatterThermostat::MatterThermostat(string szDeviceName, string szLocation) : MatterGenericDevice(szDeviceName, szLocation)
{
    ESP_LOGE("MatterThermostat","Constructor");

    DeviceType = DeviceTypeEnum::Thermostat;

    SetReachable(true);
}

int16_t MatterThermostat::GetLocalTemperature() { 
    return mLocalTempMeasurement;
} 

void MatterThermostat::SetLocalTemperature (float Value) {
    int16_t NormalizedValue = round(Value * 100);

    ESP_LOGE("?",">>>>>>>>>>>>");
    ESP_LOGE("LOCAL TEMPERATURE SET", "%f (%d) for UUID %s", Value, NormalizedValue, BridgedUUID.c_str());
    ESP_LOGE("?","<<<<<<<<<<<<");

    // Limit measurement based on the min and max.
    if (NormalizedValue < mLocalTempMin)
        NormalizedValue = mLocalTempMin;
    else if (NormalizedValue > mLocalTempMax)
        NormalizedValue = mLocalTempMax;

    bool changed = mLocalTempMeasurement != NormalizedValue;

    mLocalTempMeasurement = NormalizedValue;

    ESP_LOGE("mLocalTempMeasurement = ", "%d", mLocalTempMeasurement);

    if (changed)
        HandleStatusChanged(this, kChanged_MeasurementValue);
}

chip::app::Clusters::Thermostat::SystemModeEnum MatterThermostat::GetMode() {
    DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(BridgedUUID);
    uint8_t CurrentMode = ((DataRemote_t*)Data)->DevicesHelper.GetDeviceForType(0xEF)->GetStatusByte(IRDeviceItem.Status , 0);

    if (CurrentMode > 1)
        CurrentMode++;

    return (chip::app::Clusters::Thermostat::SystemModeEnum)CurrentMode;
}

void MatterThermostat::SetMode(chip::app::Clusters::Thermostat::SystemModeEnum ModeToSet) {
    //CurrentMode = (uint8_t)ModeToSet;
    //! Проорать о значении в HomeKit
}

float MatterThermostat::GetACTemperature() {
    DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(BridgedUUID);
    uint8_t CurrentTemp = ((DataRemote_t*)Data)->DevicesHelper.GetDeviceForType(0xEF)->GetStatusByte(IRDeviceItem.Status , 1) + 16;

    return CurrentTemp;
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
    if (itemChangedMask & (MatterTempSensor::kChanged_Reachable | MatterTempSensor::kChanged_Name | MatterTempSensor::kChanged_Location))
        HandleDeviceStatusChanged(static_cast<MatterGenericDevice *>(dev), (MatterGenericDevice::Changed_t) itemChangedMask);

    if (itemChangedMask & MatterTempSensor::kChanged_MeasurementValue)
        ScheduleReportingCallback(dev, chip::app::Clusters::Thermostat::Id, chip::app::Clusters::Thermostat::Attributes::LocalTemperature::Id);
}

Protocols::InteractionModel::Status MatterThermostat::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId attributeId, uint8_t * Buffer, uint16_t maxReadLength)
{
    ESP_LOGE("MatterThermostat", "HandleReadAttribute, ClusterID: %lu, attributeID: 0x%lx, MaxReadLength: %d", ClusterID, attributeId, maxReadLength);

    if (ClusterID == chip::app::Clusters::Thermostat::Id)
    {
        if ((attributeId == Thermostat::Attributes::LocalTemperature::Id) && (maxReadLength == 2))
        {
            ESP_LOGE("?","???????????");
            ESP_LOGE("LOCAL TEMPERATURE GET", "%d", GetLocalTemperature());
            ESP_LOGE("?","???????????");

            int16_t measuredValue = GetLocalTemperature();
            memcpy(Buffer, &measuredValue, sizeof(measuredValue));
        }
        else if (((attributeId == Thermostat::Attributes::SystemMode::Id) && (maxReadLength == 1) )
                || ((attributeId == Thermostat::Attributes::ThermostatRunningMode::Id) && (maxReadLength == 1)))
        {
            *Buffer = (uint8_t)GetMode();;
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
                    && (maxReadLength == 2))                
        {
            int16_t MaxValue = 3000;
            memcpy(Buffer, &MaxValue, sizeof(MaxValue));
        }
        else if ((attributeId == Thermostat::Attributes::OccupiedCoolingSetpoint::Id || attributeId == Thermostat::Attributes::OccupiedHeatingSetpoint::Id) && (maxReadLength == 2))
        {
            int16_t CurValue = round(GetACTemperature() * 100);
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
            return Status::Failure; 
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
            return Status::Failure; 
        }
    }

    ESP_LOGE("Thermostat", "EMBER_ZCL_STATUS_SUCCESS");
    return Status::Success;
}

Protocols::InteractionModel::Status MatterThermostat::HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value)
{
    ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Thermostat: clusterID=%lu attrId=0x%lx", ClusterID, AttributeID);

    uint8_t CurrentMode = *Value;

    if (ClusterID == chip::app::Clusters::Thermostat::Id)
    {
        ThermostatClusterWriteHandler(AttributeID, Value);
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
    }
    else
    {
        ESP_LOGE(Tag, "Wrong ClusterID: %lu", ClusterID);
    }
    bool Result = false;

    if (ClusterID == 0x201 && AttributeID == 0x1C)
        Result = HandleModeChange(*Value);

    return Result ? Status::Success : Status::Failure;
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

void MatterThermostat::ThermostatClusterWriteHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "ThermostatClusterHandler, AttributeID: %lu", AttributeID);

    if(AttributeID == 0x0000)   //Local Temperature
    {

    }
    else if (AttributeID == 0x11 || AttributeID == 0x12) // set temp for heat or cool mode
    {
        uint16_t TempToSet;
        std::memcpy(&TempToSet, Value, sizeof(TempToSet));

        // convert to IRLib value
        TempToSet = TempToSet / 100;

        ESP_LOGE("SET TEMPERATURE", "%d", TempToSet);

        if (Settings.eFuse.Type == Settings.Devices.Remote) 
        {
            DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(BridgedUUID);

            if (TempToSet > 30) TempToSet = 30;
            if (TempToSet < 16) TempToSet = 16;

            uint16_t NewStatus =  DataDeviceItem_t::SetStatusByte(IRDeviceItem.Status,1,TempToSet-16);

            CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

            if (IRCommand != nullptr) {
                string Operand = BridgedUUID + Converter::ToHexString(NewStatus,4);
                ESP_LOGE("OPERAND:", "%s", Operand.c_str());
                IRCommand->Execute(0xFE, Operand.c_str());
            }
        }
    }
    else if (AttributeID == 0x1C) // operation mode
    {
        ESP_LOGE("SET OPERATION MODE" ,"!!!!");
        ESP_LOGE(">>" ,"!!!!");

        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = ((DataRemote_t*)Data)->GetDeviceFromCache(BridgedUUID);
        // check service. If fan for ac - skip

        uint8_t NewValue = *Value;

        // OFF = 0
        // AUTO = 1
        // COOL = 3
        // HEAT = 4

        if (NewValue == 3) NewValue = 2;
        if (NewValue == 4) NewValue = 3;

        /*
            hap_val_t ValueForACFanActive;
            ValueForACFanActive.u = 0;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ACTIVE, ValueForACFanActive);

            hap_val_t ValueForACFanState;
            ValueForACFanState.f = 0;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ROTATION_SPEED, ValueForACFanState);

            hap_val_t ValueForACFanAuto;
            ValueForACFanAuto.u = 0;
            HomeKitUpdateCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_TARGET_FAN_STATE, ValueForACFanAuto);
        */

        uint16_t NewStatus =  DataDeviceItem_t::SetStatusByte(IRDeviceItem.Status,0,NewValue);

        CommandIR_t* IRCommand = (CommandIR_t *)Command_t::GetCommandByName("IR");

        if (IRCommand != nullptr) {
            string Operand = BridgedUUID + Converter::ToHexString(NewStatus,4);
            ESP_LOGE("OPERAND:", "%s", Operand.c_str());
            IRCommand->Execute(0xFE, Operand.c_str());
        }

        //Matter::StatusACUpdateIRSend(IRDeviceItem.DeviceID, IRDeviceItem.Extra,  0xE0, 0);

        ESP_LOGE("<<" ,"!!!!");
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