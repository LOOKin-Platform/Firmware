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

class MatterLight : public MatterGenericDevice {
    public:
        enum State_t
        {
            kState_On = 0,
            kState_Off,
        } State;

        MatterLight(const char * szDeviceName, const char * szLocation) : MatterGenericDevice(szDeviceName, szLocation) {};

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
                mChanged_CB(this, kChanged_State);
            }
        }

    private:
        State_t mState = kState_Off;
};