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

#define KEYPAD_FEATURE_NAVIGATION_KEYCODES     0x01
#define KEYPAD_FEATURE_LOCATION_KEYS           0x02
#define KEYPAD_FEATURE_NUMBER_KEYS             0x04
#define KEYPAD_FEATURE_MAP KEYPAD_FEATURE_NAVIGATION_KEYCODES | KEYPAD_FEATURE_LOCATION_KEYS | KEYPAD_FEATURE_NUMBER_KEYS
#define KEYPAD_CLUSTER_REVISION                 (1u)


class MatterVideoPlayer : public MatterGenericDevice {
    public:
        enum Changed_t
        {
            kChanged_MeasurementValue = kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterVideoPlayer *, MatterVideoPlayer::Changed_t)>;

        MatterVideoPlayer(string szDeviceName, string szLocation = "") :
            MatterGenericDevice(szDeviceName, szLocation)
        {
            DeviceType = DeviceTypeEnum::MediaPlayer;

            SetReachable(true);
            mChanged_CB = &MatterVideoPlayer::HandleStatusChanged;
        }

        static void HandleStatusChanged(MatterVideoPlayer * dev, MatterVideoPlayer::Changed_t itemChangedMask)
        {
            ESP_LOGE("MatterVideoPlayer", "MatterVideoPlayer");
        }

/*
        EmberAfStatus HandleWriteAttribute(chip::EndpointId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) override {
            ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Thermostat Device cluster: clusterID=%d attrId=%d", ClusterID, AttributeID);
            return EMBER_ZCL_STATUS_SUCCESS;
        }
*/

        EmberAfStatus HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override
        {
            ESP_LOGE("MatterVideoPlayer", "HandleReadAttribute, ClusterID: %lu, attributeID: 0x%lx, MaxReadLength: %d", ClusterID, AttributeID, maxReadLength);

            if (ClusterID == chip::app::Clusters::OnOff::Id)
            {
                ESP_LOGE("OnOff handler", "handler");

                if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
                {
                    *Buffer = (uint16_t) ZCL_ON_OFF_CLUSTER_REVISION;
                }
                else if ((AttributeID == OnOff::Attributes::OnOff::Id) && (maxReadLength == 1))
                {
                    *Buffer = 0;
                }
                else
                {
                    return EMBER_ZCL_STATUS_FAILURE; 
                }
            }
            
            if (ClusterID == chip::app::Clusters::KeypadInput::Id)
            {
                ESP_LOGE("KeypadInput", "handler");

                if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
                {
                    *Buffer = (uint16_t) KEYPAD_CLUSTER_REVISION;
                }
                else if ((AttributeID == FeatureMap::Id) && (maxReadLength == 4))
                {
                    uint32_t FeatureMap = KEYPAD_FEATURE_MAP;
                    memcpy(Buffer, &FeatureMap, sizeof(FeatureMap));
                }
                else
                {
                    return EMBER_ZCL_STATUS_FAILURE; 
                }
            }

            ESP_LOGE("VideoPlayer", "EMBER_ZCL_STATUS_SUCCESS");
            return EMBER_ZCL_STATUS_SUCCESS;
        }

    private:
        DeviceCallback_fn mChanged_CB;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override {
            ESP_LOGE("MatterVideoPlayer", "HandleDeviceChange");

            if (mChanged_CB)
            {
                mChanged_CB(this, (MatterVideoPlayer::Changed_t) changeMask);
            }
        }
};