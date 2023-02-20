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

#define ZCL_RELATIVE_HUMIDITY_SENSOR_FEATURE_MAP (0u)
#define ZCL_RELATIVE_HUMIDITY_SENSOR_CLUSTER_REVISION (1u)


class MatterHumiditySensor : public MatterGenericDevice {
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterHumiditySensor *, MatterHumiditySensor::Changed_t)>;

        MatterHumiditySensor(string szDeviceName, string szLocation = "", uint16_t min = 0, uint16_t max = 65535, uint16_t measuredValue = 0) :
            MatterGenericDevice(szDeviceName, szLocation),
            mMin(min), mMax(max), mMeasurement(measuredValue)
        {
            DeviceType = DeviceTypeEnum::Humidity;

            SetReachable(true);
            mChanged_CB = &MatterHumiditySensor::HandleStatusChanged;
        }

        int16_t GetHumidity() { 
            return mMeasurement;
        } 

        void SetHumidity (float Value) {
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

            if (changed && mChanged_CB)
            {
                mChanged_CB(this, kChanged_MeasurementValue);
            }
        }

        uint16_t GetMin() { return mMin; }
        uint16_t GetMax() { return mMax; }

        static void HandleStatusChanged(MatterHumiditySensor * dev, MatterHumiditySensor::Changed_t itemChangedMask)
        {
            if (itemChangedMask & (MatterHumiditySensor::kChanged_Reachable | MatterHumiditySensor::kChanged_Name | MatterHumiditySensor::kChanged_Location))
                HandleDeviceStatusChanged(static_cast<MatterGenericDevice *>(dev), (MatterGenericDevice::Changed_t) itemChangedMask);

            if (itemChangedMask & MatterHumiditySensor::kChanged_MeasurementValue)
                ScheduleReportingCallback(dev, chip::app::Clusters::RelativeHumidityMeasurement::Id, chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);
        }


        EmberAfStatus HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override
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


    private:
        const uint16_t   mMin;
        const uint16_t   mMax;
        uint16_t         mMeasurement = 0;

        DeviceCallback_fn mChanged_CB;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override {
            if (mChanged_CB)
            {
                mChanged_CB(this, (MatterHumiditySensor::Changed_t) changeMask);
            }
        }
};