/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#ifndef MATTER_DEVICES
#define MATTER_DEVICES

// These are the bridged devices
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include "Matter.h"

#include <app/util/attribute-storage.h>

#include <platform/CHIPDeviceLayer.h>
#include <lib/support/CHIPMemString.h>

#include <app/reporting/reporting.h>

#include <functional>
#include <stdbool.h>
#include <stdint.h>

#include <typeinfo>

static const int kNodeLabelSize = 32;
static const int kDescriptorAttributeArraySize = 254;

using namespace chip::Platform;
class MatterGenericDevice
{
    public:
        enum DeviceTypeEnum {
            Undefined   = 0x0,
            Light,
            Thermostat,
            Temperature,
            Humidity,
            MediaPlayer 
        };

        static const int        kDeviceNameSize     = 32;
        static const int        kDeviceLocationSize = 32;

        static inline bool      IsBridgedDevice = false;
        static inline string    BridgedUUID = "";
        static inline uint8_t   BridgedMisc = 0x0;

        //chip::Span<chip::DataVersion> dataVersions;
        //vector<chip::DataVersion> dataVersions = vector<chip::DataVersion>();
        chip::DataVersion dataVersions[5] = {0};

        uint16_t DynamicIndex               = 0;

        enum Changed_t
        {
            kChanged_Reachable = 0x01,
            kChanged_State     = 0x02,
            kChanged_Location  = 0x04,
            kChanged_Name      = 0x08,
            kChanged_Last      = kChanged_Name,
        } Changed;

        MatterGenericDevice(string szDeviceName, string szLocation)
        {
            CopyString(mName, sizeof(mName), szDeviceName.c_str());
            CopyString(mLocation, sizeof(mLocation), szLocation.c_str());

            mReachable  = false;
            mEndpointId = 0xFFFF;
            mChanged_CB = nullptr;

            //dataVersions.reserve(3);
        }

        bool IsReachable() const {
            return mReachable;
        }

        void SetReachable(bool aReachable) {
            bool changed = (mReachable != aReachable);

            mReachable = aReachable;

            if (aReachable)
            {
                ChipLogProgress(DeviceLayer, "Device[%s]: ONLINE", mName);
            }
            else
            {
                ChipLogProgress(DeviceLayer, "Device[%s]: OFFLINE", mName);
            }

            if (changed && mChanged_CB)
            {
                mChanged_CB(this, kChanged_Reachable);
            }   
        }

        void SetName(const char * szDeviceName) {
            bool changed = (strncmp(mName, szDeviceName, sizeof(mName)) != 0);

            ChipLogProgress(DeviceLayer, "Device[%s]: New Name=\"%s\"", mName, szDeviceName);

            CopyString(mName, sizeof(mName), szDeviceName);

            if (changed && mChanged_CB)
            {
                mChanged_CB(this, kChanged_Name);
            }
        }

        void SetLocation(const char * szLocation) {
            bool changed = (strncmp(mLocation, szLocation, sizeof(mLocation)) != 0);

            CopyString(mLocation, sizeof(mLocation), szLocation);

            ChipLogProgress(DeviceLayer, "Device[%s]: Location=\"%s\"", mName, mLocation);

            if (changed && mChanged_CB)
            {
                mChanged_CB(this, kChanged_Location);
            }
        }

        inline void             SetEndpointID(chip::EndpointId id)  { mEndpointId = id; };
        inline chip::EndpointId GetEndpointID()                     { return mEndpointId; };
        inline char *           GetName()                           { return mName; };
        inline char *           GetLocation()                       { return mLocation; };
        inline DeviceTypeEnum   GetTypeName()                       { return DeviceType; };

        using DeviceCallback_fn = std::function<void(MatterGenericDevice *, Changed_t)>;
        void SetChangeCallback(DeviceCallback_fn aChanged_CB) {
            mChanged_CB = aChanged_CB;
        }

        virtual EmberAfStatus HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) 
        {
            return EMBER_ZCL_STATUS_FAILURE;
        }

        virtual EmberAfStatus HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) 
        {
            return EMBER_ZCL_STATUS_SUCCESS;
        }

        virtual bool    GetOnOff()                          { ESP_LOGE("GetOnOff Generic", "Invoked"); return false;}
        virtual void    SetOnOff(bool Value)                { ESP_LOGE("SetOnOff Generic", "Invoked"); }

    protected:
        static inline DeviceTypeEnum DeviceType = Undefined;


        char mLocation[kDeviceLocationSize];
        char mName[kDeviceNameSize];
        DeviceCallback_fn mChanged_CB;

        static void HandleDeviceStatusChanged(MatterGenericDevice * dev, MatterGenericDevice::Changed_t itemChangedMask)
        {
            ESP_LOGE("MatterDevice", "HandleDeviceStatusChanged");

            if (itemChangedMask & MatterGenericDevice::kChanged_Reachable)
                ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters:: BridgedDeviceBasicInformation::Attributes::Reachable::Id);

            if (itemChangedMask & MatterGenericDevice::kChanged_Name)
                ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
        }

        static void ScheduleReportingCallback(MatterGenericDevice * dev, chip::ClusterId cluster, chip::AttributeId attribute)
        {
            auto * path = chip::Platform::New<chip::app::ConcreteAttributePath>(dev->GetEndpointID(), cluster, attribute);
            chip::DeviceLayer::PlatformMgr().ScheduleWork(CallReportingCallback, reinterpret_cast<intptr_t>(path));
        }

    private:
        bool mReachable;
        chip::EndpointId mEndpointId;

        virtual void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) = 0;

        const char *Tag = "MatterDevice";

        static void CallReportingCallback(intptr_t closure)
        {
            auto path = reinterpret_cast<chip::app::ConcreteAttributePath *>(closure);
            MatterReportingAttributeChangeCallback(*path);
            chip::Platform::Delete(path);
        }

};

//#include "GenericDevice.hpp"

#include "Light.hpp"
#include "Outlet.hpp"
#include "TempSensor.hpp"
#include "HumiditySensor.hpp"
#include "Thermostat.hpp"
#include "VideoPlayer.hpp"

#endif