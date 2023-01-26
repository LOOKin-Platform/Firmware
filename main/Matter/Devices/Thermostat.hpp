/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "Matter.h"
#include "GenericDevice.hpp"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

#include <math.h>

#define THERMOSTAT_FEATURE_MAP_HEAT     0x01
#define THERMOSTAT_FEATURE_MAP_COOL     0x02
#define THERMOSTAT_FEATURE_MAP_OCC      0x04
#define THERMOSTAT_FEATURE_MAP_SCH      0x08
#define THERMOSTAT_FEATURE_MAP_SB       0x10
#define THERMOSTAT_FEATURE_MAP_AUTO     0x20
#define ZCL_THERMOSTAT_FEATURE_MAP THERMOSTAT_FEATURE_MAP_HEAT | THERMOSTAT_FEATURE_MAP_COOL | THERMOSTAT_FEATURE_MAP_AUTO
#define ZCL_THERMOSTAT_CLUSTER_REVISION (5u)

#define FANCONTROL_FEATURE_MULTISPEED       0x01
#define FANCONTROL_FEATURE_AUTO_SUPPORTED   0x02
#define FANCONTROL_FEATURE_ROCKING          0x04
#define FANCONTROL_FEATURE_WIND_SUPPORTED   0x08

#define ZCL_FANCONTROL_FEATURE_MAP FANCONTROL_FEATURE_MULTISPEED | FANCONTROL_FEATURE_AUTO_SUPPORTED | FANCONTROL_FEATURE_ROCKING
#define ZCL_FANCONTROL_CLUSTER_REVISION (2u)

class MatterThermostat : public MatterGenericDevice {
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterThermostat *, MatterThermostat::Changed_t)>;

        MatterThermostat(string szDeviceName, string szLocation) :
            MatterGenericDevice(szDeviceName, szLocation)
        {
            DeviceType = DeviceTypeEnum::Thermostat;

            SetReachable(true);
            mChanged_CB = &MatterThermostat::HandleStatusChanged;
        }

        static void HandleStatusChanged(MatterThermostat * dev, MatterThermostat::Changed_t itemChangedMask)
        {
            ESP_LOGE("MatterThermostat", "MatterThermostat");
        }

/*
        EmberAfStatus HandleWriteAttribute(chip::EndpointId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) override {
            ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Thermostat Device cluster: clusterID=%d attrId=%d", ClusterID, AttributeID);
            return EMBER_ZCL_STATUS_SUCCESS;
        }
*/

        EmberAfStatus HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId attributeId, uint8_t * buffer, uint16_t maxReadLength) override
        {
            ESP_LOGE("MatterThermostat", "HandleReadAttribute, ClusterID: %d, attributeID: %04X, MaxReadLength: %d", ClusterID, attributeId, maxReadLength);

            if (ClusterID == chip::app::Clusters::Thermostat::Id)
            {
                ESP_LOGE("THERMOSTAT", "handler");
                if ((attributeId == ZCL_LOCAL_TEMPERATURE_ATTRIBUTE_ID) && (maxReadLength == 2))
                {
                    int16_t measuredValue = 1700;
                    memcpy(buffer, &measuredValue, sizeof(measuredValue));
                }
                else if ((attributeId == ZCL_SYSTEM_MODE_ATTRIBUTE_ID) && (maxReadLength == 1))
                {
                    uint8_t CurrentMode = static_cast<uint8_t>(chip::app::Clusters::Thermostat::ThermostatSystemMode::kCool);                
                    *buffer = CurrentMode;
                }
                else if ((attributeId == ZCL_THERMOSTAT_RUNNING_MODE_ATTRIBUTE_ID) && (maxReadLength == 1))
                {
                    uint8_t CurrentMode = static_cast<uint8_t>(chip::app::Clusters::Thermostat::ThermostatSystemMode::kCool);
                    *buffer = CurrentMode;
                }
                else if ((     attributeId == ZCL_MIN_COOL_SETPOINT_LIMIT_ATTRIBUTE_ID 
                            || attributeId == ZCL_MIN_HEAT_SETPOINT_LIMIT_ATTRIBUTE_ID
                            || attributeId == ZCL_ABS_MIN_HEAT_SETPOINT_LIMIT_ATTRIBUTE_ID
                            || attributeId == ZCL_ABS_MIN_COOL_SETPOINT_LIMIT_ATTRIBUTE_ID) 
                            && (maxReadLength == 2))
                {
                    int16_t MinValue = 1600;
                    memcpy(buffer, &MinValue, sizeof(MinValue));
                }
                else if ((     attributeId == ZCL_MAX_COOL_SETPOINT_LIMIT_ATTRIBUTE_ID 
                            || attributeId == ZCL_MAX_HEAT_SETPOINT_LIMIT_ATTRIBUTE_ID
                            || attributeId == ZCL_ABS_MAX_HEAT_SETPOINT_LIMIT_ATTRIBUTE_ID
                            || attributeId == ZCL_ABS_MAX_COOL_SETPOINT_LIMIT_ATTRIBUTE_ID) 
                            && (maxReadLength == 2))                {
                    int16_t MaxValue = 3000;
                    memcpy(buffer, &MaxValue, sizeof(MaxValue));
                }
                else if ((attributeId == ZCL_OCCUPIED_COOLING_SETPOINT_ATTRIBUTE_ID || attributeId == ZCL_OCCUPIED_HEATING_SETPOINT_ATTRIBUTE_ID) && (maxReadLength == 2))
                {
                    int16_t CurValue = 2100;
                    memcpy(buffer, &CurValue, sizeof(CurValue));
                }
                else if ((attributeId == ZCL_FEATURE_MAP_SERVER_ATTRIBUTE_ID) && (maxReadLength == 4))
                {
                    uint32_t FeatureMap = ZCL_THERMOSTAT_FEATURE_MAP;
                    memcpy(buffer, &FeatureMap, sizeof(FeatureMap));
                }
                else if ((attributeId == ZCL_CLUSTER_REVISION_SERVER_ATTRIBUTE_ID) && (maxReadLength == 2))
                {
                    uint16_t ClusterRevision = ZCL_THERMOSTAT_CLUSTER_REVISION;
                    memcpy(buffer, &ClusterRevision, sizeof(ClusterRevision));
                }
                else
                {
                    return EMBER_ZCL_STATUS_FAILURE; 
                }
            }
            
            if (ClusterID == chip::app::Clusters::FanControl::Id) // Fan Control
            {
                ESP_LOGE("FAN", "handler");

                if ((attributeId == ZCL_FAN_MODE_ATTRIBUTE_ID) && (maxReadLength == 1))
                {
                    uint8_t CurrentFanMode = 5; // Auto                
                    *buffer = CurrentFanMode;
                }
                else if ((attributeId == ZCL_CONTROL_SEQUENCE_OF_OPERATION_ATTRIBUTE_ID) && (maxReadLength == 1)) 
                {
                    uint8_t ControlSequence = 0x4;
                    *buffer = ControlSequence;
                }
                else if ((attributeId == ZCL_PERCENT_SETTING_ATTRIBUTE_ID || attributeId == ZCL_PERCENT_CURRENT_ATTRIBUTE_ID) && (maxReadLength == 1)) 
                {
                    uint8_t CurrentValue = 0x1;
                    *buffer = CurrentValue;
                }
                else if ((attributeId == ZCL_FEATURE_MAP_SERVER_ATTRIBUTE_ID) && (maxReadLength == 4))
                {
                    uint32_t FanFeatureMap = ZCL_FANCONTROL_FEATURE_MAP;
                    memcpy(buffer, &FanFeatureMap, sizeof(FanFeatureMap));
                }
                else if ((attributeId == ZCL_CLUSTER_REVISION_SERVER_ATTRIBUTE_ID) && (maxReadLength == 2))
                {
                    uint16_t ClusterRevision = ZCL_FANCONTROL_CLUSTER_REVISION;
                    memcpy(buffer, &ClusterRevision, sizeof(ClusterRevision));
                }
                else if (((attributeId == ZCL_ROCK_SUPPORT_ATTRIBUTE_ID) || (attributeId == ZCL_ROCK_SETTING_ATTRIBUTE_ID)) && (maxReadLength == 1))
                {
                    uint8_t RockSettings = 0x2;  // Rock setting             
                    *buffer = RockSettings;
                }
                else if ((attributeId == ZCL_FAN_MODE_SEQUENCE_ATTRIBUTE_ID) && (maxReadLength == 1))
                {
                    uint8_t FanModeSequence = 2; // Off/Low/Med/High/Auto         
                    *buffer = FanModeSequence;
                }
                else if ((attributeId == ZCL_SPEED_SETTING_ATTRIBUTE_ID) && (maxReadLength == 1))
                {
                    uint8_t SpeedSetting = 0x0;  // Vertical Swing              
                    *buffer = SpeedSetting;
                }
                else if ((attributeId == ZCL_PERCENT_CURRENT_ATTRIBUTE_ID) && (maxReadLength == 1))
                {
                    uint8_t SpeedSetting = 0x4;             
                    *buffer = SpeedSetting;
                }                
                else
                {
                    return EMBER_ZCL_STATUS_FAILURE; 
                }
            }

            ESP_LOGE("Thermostat", "EMBER_ZCL_STATUS_SUCCESS");
            return EMBER_ZCL_STATUS_SUCCESS;
        }

    private:
        DeviceCallback_fn mChanged_CB;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override {
            ESP_LOGE("MatterThermostat", "HandleDeviceChange");

            if (mChanged_CB)
            {
                mChanged_CB(this, (MatterThermostat::Changed_t) changeMask);
            }
        }
};