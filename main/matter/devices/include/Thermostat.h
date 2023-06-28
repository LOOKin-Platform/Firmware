#ifndef MATTER_DEVICES_THERMOSTAT
#define MATTER_DEVICES_THERMOSTAT

#include "GenericDevice.h"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

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

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

class MatterThermostat : public MatterGenericDevice {
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = MatterGenericDevice::Changed_t::kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterThermostat *, MatterThermostat::Changed_t)>;

        MatterThermostat(string szDeviceName, string szLocation = "");

        int16_t         GetLocalTemperature();
        void            SetLocalTemperature (float Value);

        chip::app::Clusters::Thermostat::ThermostatSystemMode GetMode();
        void            SetMode(chip::app::Clusters::Thermostat::ThermostatSystemMode ModeToSet);

        float           GetACTemperature();
        void            SetACTemperature(float Value);

        static void     HandleStatusChanged(MatterThermostat * dev, MatterThermostat::Changed_t itemChangedMask);
        EmberAfStatus   HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId attributeId, uint8_t * Buffer, uint16_t maxReadLength) override;
        EmberAfStatus   HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) override;
    
    private:
        const int16_t   mLocalTempMin           = 10000;
        const int16_t   mLocalTempMax           = -10000;

        int16_t         mLocalTempMeasurement   = 1600;

        int16_t         ACTemp                  = 2100;
        uint8_t         CurrentMode             = 0;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override;
        bool HandleModeChange(uint8_t Value);

        void ThermostatClusterWriteHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void TemperatureMeasurementClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void ThermostatUIConfigClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void FanControlClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void WriteThermostatOperatingStateClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);

};

#endif