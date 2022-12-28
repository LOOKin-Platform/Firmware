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

#ifndef MATTER_GENERIC_DEVICE
#define MATTER_GENERIC_DEVICE

// These are the bridged devices
#include <app/util/attribute-storage.h>
#include <functional>
#include <stdbool.h>
#include <stdint.h>

using namespace ::chip::Platform;

class MatterGenericDevice
{
public:
    static const int kDeviceNameSize     = 32;
    static const int kDeviceLocationSize = 32;

    enum Changed_t
    {
        kChanged_Reachable = 0x01,
        kChanged_State     = 0x02,
        kChanged_Location  = 0x04,
        kChanged_Name      = 0x08,
    } Changed;

    MatterGenericDevice(const char * szDeviceName, const char * szLocation)
    {
        CopyString(mName, sizeof(mName), szDeviceName);
        CopyString(mLocation, sizeof(mLocation), szLocation);
        mReachable  = false;
        mEndpointId = 0;
        mChanged_CB = nullptr;
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

    inline void SetEndpointId(chip::EndpointId id)  { mEndpointId = id; };
    inline chip::EndpointId GetEndpointId()         { return mEndpointId; };
    inline char * GetName()                         { return mName; };
    inline char * GetLocation()                     { return mLocation; };

    using DeviceCallback_fn = std::function<void(MatterGenericDevice *, Changed_t)>;
    void SetChangeCallback(DeviceCallback_fn aChanged_CB) {
        mChanged_CB = aChanged_CB;
    }

protected:
    char mLocation[kDeviceLocationSize];
    char mName[kDeviceNameSize];
    DeviceCallback_fn mChanged_CB;

private:
    bool mReachable;
    chip::EndpointId mEndpointId;
};

#include "Light.hpp"

#endif