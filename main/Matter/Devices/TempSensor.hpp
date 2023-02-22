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

using namespace ::chip::app::Clusters;

#define ZCL_TEMPERATURE_SENSOR_CLUSTER_REVISION 	(1u)
#define ZCL_TEMPERATURE_SENSOR_FEATURE_MAP 			(0u)

class MatterTempSensor : public MatterGenericDevice {
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterTempSensor *, MatterTempSensor::Changed_t)>;

        MatterTempSensor(string szDeviceName, string szLocation = "", int16_t measuredValue = 0, int16_t min = -10000, int16_t max = 10000) :
            MatterGenericDevice(szDeviceName, szLocation),
            mMeasurement(measuredValue), mMin(min), mMax(max)
        {
            DeviceType = DeviceTypeEnum::Temperature;

            SetReachable(true);
        }

        int16_t GetTemperature() { 
            return mMeasurement;
        } 

        void SetTemperature (float Value, bool ShouldInvokeStatusChanged = true) {
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

        int16_t GetMin() { return mMin; }
        int16_t GetMax() { return mMax; }

        static void HandleStatusChanged(MatterTempSensor * dev, MatterTempSensor::Changed_t itemChangedMask)
        {
            if (itemChangedMask & (MatterTempSensor::kChanged_Reachable | MatterTempSensor::kChanged_Name | MatterTempSensor::kChanged_Location))
                HandleDeviceStatusChanged(static_cast<MatterGenericDevice *>(dev), (MatterGenericDevice::Changed_t) itemChangedMask);

            if (itemChangedMask & MatterTempSensor::kChanged_MeasurementValue)
                ScheduleReportingCallback(dev, chip::app::Clusters::TemperatureMeasurement::Id, chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id);
        }

        EmberAfStatus HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override
        {
            ESP_LOGE("HandleReadAttribute TEMP SENSOR", "%d %d %d", ClusterID, AttributeID, maxReadLength);

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

    private:
        int16_t         mMeasurement = 0;

        const int16_t   mMin;
        const int16_t   mMax;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override {
            HandleStatusChanged(this, (MatterTempSensor::Changed_t) changeMask);
        }
};