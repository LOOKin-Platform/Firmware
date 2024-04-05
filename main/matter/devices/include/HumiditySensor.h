#ifndef MATTER_DEVICES_HUMIDITYSENSOR
#define MATTER_DEVICES_HUMIDITYSENSOR

#include "GenericDevice.h"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#define ZCL_RELATIVE_HUMIDITY_SENSOR_FEATURE_MAP (0u)
#define ZCL_RELATIVE_HUMIDITY_SENSOR_CLUSTER_REVISION (1u)

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

class MatterHumiditySensor : public MatterGenericDevice {
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = MatterGenericDevice::Changed_t::kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterHumiditySensor *, MatterHumiditySensor::Changed_t)>;

        MatterHumiditySensor(string szDeviceName, string szLocation = "", uint16_t min = 0, uint16_t max = 65535, uint16_t measuredValue = 0);
        
        int16_t GetHumidity();
        void    SetHumidity (float Value, bool ShouldInvokeStatusChanged = true);

        uint16_t GetMin();
        uint16_t GetMax();
        
        static void     HandleStatusChanged(MatterHumiditySensor * dev, MatterHumiditySensor::Changed_t itemChangedMask);
        Protocols::InteractionModel::Status   HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override;
    
    private:
        const uint16_t   mMin;
        const uint16_t   mMax;
        uint16_t         mMeasurement = 0;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override;
};

#endif