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

#include "GenericDevice.hpp"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

#define ZCL_ON_OFF_CLUSTER_REVISION (4u)

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

class MatterLight : public MatterGenericDevice {
    public:
        enum State_t
        {
            kState_On = 0,
            kState_Off,
        } State;

        enum Changed_t
        {
            kChanged_OnOff = kChanged_Last << 1,
        } Changed;

        using DeviceCallback_fn = std::function<void(MatterLight *, MatterLight::Changed_t)>;

        MatterLight(string szDeviceName, string szLocation = "") : MatterGenericDevice(szDeviceName, szLocation) {
            DeviceType = MatterGenericDevice::Light;

            SetReachable(true);
            mChanged_CB = &MatterLight::HandleStatusChanged;
        };

        EmberAfStatus HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override
        {
            ESP_LOGE("Light", "HandleReadOnOffAttribute: attrId=%d, maxReadLength=%d", AttributeID, maxReadLength);

            if ((AttributeID == OnOff::Attributes::OnOff::Id) && (maxReadLength == 1))
            {
                *Buffer = GetOnOff() ? 1 : 0;
            }
            else if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
            {
                *Buffer = (uint16_t) ZCL_ON_OFF_CLUSTER_REVISION;
            }
            else
            {
                return EMBER_ZCL_STATUS_FAILURE;
            }

            return EMBER_ZCL_STATUS_SUCCESS;
        }

        EmberAfStatus HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) override {
            ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Light cluster: clusterID=%d attrId=%d", ClusterID, AttributeID);

            ReturnErrorCodeIf((AttributeID != OnOff::Attributes::OnOff::Id) || (!IsReachable()), EMBER_ZCL_STATUS_FAILURE);
                        
            SetOnOff(*Value == 1);
            return EMBER_ZCL_STATUS_SUCCESS;
        }

        bool GetOnOff()  override  { 
            ESP_LOGE("Light device GetOnOff", "Invoked"); 
            return (mState == kState_On);
        }

        void SetOnOff(bool aOn) override {
            bool changed;

            if (aOn)
            {
                changed = (mState != kState_On);
                mState  = kState_On;
                ChipLogProgress(DeviceLayer, "Device[%s]: ON", mName);
            }
            else
            {
                changed = (mState != kState_Off);
                mState  = kState_Off;
                ChipLogProgress(DeviceLayer, "Device[%s]: OFF", mName);
            }

            if (changed && mChanged_CB)
            {
                mChanged_CB(this, kChanged_OnOff);
            }
        }

        static void HandleStatusChanged(MatterLight * dev, MatterLight::Changed_t itemChangedMask)
        {
            if (itemChangedMask & MatterGenericDevice::kChanged_Reachable)
                ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id);

            if (itemChangedMask & MatterGenericDevice::kChanged_State)
                ScheduleReportingCallback(dev, chip::app::Clusters::OnOff::Id, chip::app::Clusters::OnOff::Attributes::OnOff::Id);

            if (itemChangedMask & MatterGenericDevice::kChanged_Name)
                ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
        }

    private:
        State_t mState = kState_Off;

        DeviceCallback_fn mChanged_CB;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override
        {
            if (mChanged_CB)
            {
                mChanged_CB(this, (MatterLight::Changed_t) changeMask);
            }
        }
};