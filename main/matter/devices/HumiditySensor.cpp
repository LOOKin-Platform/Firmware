#include "HumiditySensor.h"

#include <math.h>

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

MatterHumiditySensor::MatterHumiditySensor(string szDeviceName, string szLocation, uint16_t min, uint16_t max, uint16_t measuredValue) 
    : MatterGenericDevice(szDeviceName, szLocation), mMin(min), mMax(max), mMeasurement(measuredValue)
{
    DeviceType = DeviceTypeEnum::Humidity;

    SetReachable(true);
}

int16_t MatterHumiditySensor::GetHumidity() { 
    return mMeasurement;
} 

void MatterHumiditySensor::SetHumidity(float Value, bool ShouldInvokeStatusChanged) {
    if (Value < 0)
        Value = 0;

    uint16_t NormalizedValue = abs(round(Value * 100));

    // Limit measurement based on the min and max.
    if (NormalizedValue < mMin)
        NormalizedValue = mMin;

    else if (NormalizedValue > mMax)
        NormalizedValue = mMax;

    bool changed = mMeasurement != NormalizedValue;

    mMeasurement = NormalizedValue;

    if (changed && ShouldInvokeStatusChanged)
        HandleStatusChanged(this, kChanged_MeasurementValue);
}

uint16_t MatterHumiditySensor::GetMin() 
{ 
    return mMin; 
}

uint16_t MatterHumiditySensor::GetMax() 
{ 
    return mMax; 
}

void MatterHumiditySensor::HandleStatusChanged(MatterHumiditySensor * dev, MatterHumiditySensor::Changed_t itemChangedMask)
{
    if (itemChangedMask & (MatterHumiditySensor::kChanged_Reachable | MatterHumiditySensor::kChanged_Name | MatterHumiditySensor::kChanged_Location))
        HandleDeviceStatusChanged(static_cast<MatterGenericDevice *>(dev), (MatterGenericDevice::Changed_t) itemChangedMask);

    if (itemChangedMask & MatterHumiditySensor::kChanged_MeasurementValue)
        ScheduleReportingCallback(dev, chip::app::Clusters::RelativeHumidityMeasurement::Id, chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);
}


EmberAfStatus MatterHumiditySensor::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength)
{
    if ((AttributeID == RelativeHumidityMeasurement::Attributes::MeasuredValue::Id) && (maxReadLength == 2))
    {
        uint16_t measuredValue = GetHumidity();
        memcpy(Buffer, &measuredValue, sizeof(measuredValue));
    }
    else if ((AttributeID == RelativeHumidityMeasurement::Attributes::MinMeasuredValue::Id) && (maxReadLength == 2))
    {
        uint16_t minValue = GetMin();
        memcpy(Buffer, &minValue, sizeof(minValue));
    }
    else if ((AttributeID == RelativeHumidityMeasurement::Attributes::MaxMeasuredValue::Id) && (maxReadLength == 2))
    {
        uint16_t maxValue = GetMax();
        memcpy(Buffer, &maxValue, sizeof(maxValue));
    }
    else if ((AttributeID == RelativeHumidityMeasurement::Attributes::FeatureMap::Id) && (maxReadLength == 4))
    {
        uint32_t featureMap = ZCL_RELATIVE_HUMIDITY_SENSOR_FEATURE_MAP;
        memcpy(Buffer, &featureMap, sizeof(featureMap));
    }
    else if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
    {
        uint16_t ClusterRevision = ZCL_RELATIVE_HUMIDITY_SENSOR_CLUSTER_REVISION;
        memcpy(Buffer, &ClusterRevision, sizeof(ClusterRevision));
    }
    else
    {
        return EMBER_ZCL_STATUS_FAILURE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

void MatterHumiditySensor::HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask)
{
    HandleStatusChanged(this, (MatterHumiditySensor::Changed_t) changeMask);
}