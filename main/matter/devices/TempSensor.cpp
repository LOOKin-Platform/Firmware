#include "TempSensor.h"

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

MatterTempSensor::MatterTempSensor(string szDeviceName, string szLocation, int16_t measuredValue, int16_t min, int16_t max) 
    : MatterGenericDevice(szDeviceName, szLocation), mMeasurement(measuredValue), mMin(min), mMax(max)
{
    DeviceType = DeviceTypeEnum::Temperature;

    SetReachable(true);
}

int16_t MatterTempSensor::GetTemperature() { 
    return mMeasurement;
} 

void MatterTempSensor::SetTemperature (float Value, bool ShouldInvokeStatusChanged) {
    ESP_LOGE("TEMP VALUE", "%f", Value);

    int16_t NormalizedValue = round(Value * 100);

    // Limit measurement based on the min and max.
    if (NormalizedValue < mMin)
        NormalizedValue = mMin;
    else if (NormalizedValue > mMax)
        NormalizedValue = mMax;

    bool changed = mMeasurement != NormalizedValue;

    mMeasurement = NormalizedValue;

    ESP_LOGE("mMeasurement = ", "%d", mMeasurement);

    if (changed && ShouldInvokeStatusChanged)
        HandleStatusChanged(this, kChanged_MeasurementValue);
}

int16_t MatterTempSensor::GetMin() { return mMin; }
int16_t MatterTempSensor::GetMax() { return mMax; }

void MatterTempSensor::HandleStatusChanged(MatterTempSensor * dev, MatterTempSensor::Changed_t itemChangedMask)
{
    if (itemChangedMask & (MatterTempSensor::kChanged_Reachable | MatterTempSensor::kChanged_Name | MatterTempSensor::kChanged_Location))
        HandleDeviceStatusChanged(static_cast<MatterGenericDevice *>(dev), (MatterGenericDevice::Changed_t) itemChangedMask);

    if (itemChangedMask & MatterTempSensor::kChanged_MeasurementValue)
        ScheduleReportingCallback(dev, chip::app::Clusters::TemperatureMeasurement::Id, chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id);
}

EmberAfStatus MatterTempSensor::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength)
{
    ESP_LOGE("HandleReadAttribute TEMP SENSOR", "%lu 0x%lx %d", ClusterID, AttributeID, maxReadLength);

    if ((AttributeID == TemperatureMeasurement::Attributes::MeasuredValue::Id) && (maxReadLength == 2))
    {             
        int16_t measuredValue = GetTemperature();
        
        ESP_LOGE("MEASURED VALUE", "%d", measuredValue);

        memcpy(Buffer, &measuredValue, sizeof(measuredValue));
    }
    else if ((AttributeID == TemperatureMeasurement::Attributes::MinMeasuredValue::Id) && (maxReadLength == 2))
    {
        int16_t minValue = GetMin();
        memcpy(Buffer, &minValue, sizeof(minValue));
    }
    else if ((AttributeID == TemperatureMeasurement::Attributes::MaxMeasuredValue::Id) && (maxReadLength == 2))
    {
        int16_t maxValue = GetMax();
        memcpy(Buffer, &maxValue, sizeof(maxValue));
    }
    else if ((AttributeID == TemperatureMeasurement::Attributes::FeatureMap::Id) && (maxReadLength == 4))
    {
        uint32_t featureMap = ZCL_TEMPERATURE_SENSOR_FEATURE_MAP;
        memcpy(Buffer, &featureMap, sizeof(featureMap));
    }
    else if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
    {
        uint16_t clusterRevision = ZCL_TEMPERATURE_SENSOR_CLUSTER_REVISION;
        memcpy(Buffer, &clusterRevision, sizeof(clusterRevision));
    }
    else
    {
        return EMBER_ZCL_STATUS_FAILURE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

void MatterTempSensor::HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask)
{
    HandleStatusChanged(this, (MatterTempSensor::Changed_t) changeMask);
}
