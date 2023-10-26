#ifndef MATTER_DEVICES_TEMP_SENSOR
#define MATTER_DEVICES_TEMP_SENSOR

#include "GenericDevice.h"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

#include <math.h>

using namespace ::chip::app::Clusters;

#define ZCL_TEMPERATURE_SENSOR_CLUSTER_REVISION 	(1u)
#define ZCL_TEMPERATURE_SENSOR_FEATURE_MAP 			(0u)

class MatterTempSensor : public MatterGenericDevice 
{
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = MatterGenericDevice::Changed_t::kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterTempSensor *, MatterTempSensor::Changed_t)>;

        MatterTempSensor(string szDeviceName, string szLocation = "", int16_t measuredValue = 0, int16_t min = -10000, int16_t max = 10000);

        int16_t         GetTemperature();
        void            SetTemperature (float Value, bool ShouldInvokeStatusChanged = true);

        int16_t         GetMin();
        int16_t         GetMax();

        static void     HandleStatusChanged(MatterTempSensor * dev, MatterTempSensor::Changed_t itemChangedMask);
        EmberAfStatus   HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override;

    private:
        int16_t         mMeasurement = 0;

        const int16_t   mMin = 0;
        const int16_t   mMax = 10000;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override;
};

#endif