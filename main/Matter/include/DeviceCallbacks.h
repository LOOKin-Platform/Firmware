/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
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

/**
 * @file DeviceCallbacks.h
 *
 * Implementations for the DeviceManager callbacks for this application
 *
 **/

#pragma once

#include <common/CHIPDeviceManager.h>
#include <common/CommonDeviceCallbacks.h>

#include <esp_log.h>

class AppDeviceCallbacks : public CommonDeviceCallbacks
{
public:
    virtual void PostAttributeChangeCallback(chip::EndpointId endpointId, chip::ClusterId clusterId, chip::AttributeId attributeId,
                                             uint8_t type, uint16_t size, uint8_t * value);

private:
    void OnOnOffPostAttributeChangeCallback(chip::EndpointId endpointId, chip::AttributeId attributeId, uint8_t * value);
    void OnLevelControlAttributeChangeCallback(chip::EndpointId endpointId, chip::AttributeId attributeId, uint8_t * value);
#if CONFIG_DEVICE_TYPE_ESP32_C3_DEVKITM
    void OnColorControlAttributeChangeCallback(chip::EndpointId endpointId, chip::AttributeId attributeId, uint8_t * value);
#endif

    void OnIdentifyPostAttributeChangeCallback(chip::EndpointId endpointId, chip::AttributeId attributeId, uint8_t * value);

    bool mEndpointOnOffState[2];

    void OnCommissioningSessionStarted()
    {
        ESP_LOGE("!","OnCommissioningSessionStarted");
//        PostEvent(chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted);
    }

    void OnCommissioningSessionStopped()
    {
        ESP_LOGE("!","OnCommissioningSessionStopped");
        //PostEvent(chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped);
    }

    void OnCommissioningWindowOpened()
    {
        ESP_LOGE("!","OnCommissioningWindowOpened");

        //PostEvent(chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened);
    }

    void OnCommissioningWindowClosed()
    {
        ESP_LOGE("!","OnCommissioningWindowClosed");

        //PostEvent(chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed);
    }

};

class AppDeviceCallbacksDelegate : public DeviceCallbacksDelegate
{
public:
    void OnIPv4ConnectivityEstablished(void) override;
    void OnIPv4ConnectivityLost(void) override;
};
