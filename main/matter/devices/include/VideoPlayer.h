#ifndef MATTER_DEVICES_VIDEO_PLAYER
#define MATTER_DEVICES_VIDEO_PLAYER

#include "Matter.h"
#include "GenericDevice.h"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

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

        MatterVideoPlayer(string szDeviceName, string szLocation = "");

        static void     HandleStatusChanged(MatterVideoPlayer * dev, MatterVideoPlayer::Changed_t itemChangedMask);
        Protocols::InteractionModel::Status   HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override;

    private:
        DeviceCallback_fn mChanged_CB;
        
        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override;


        void OnOffClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void BasicClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void IdentifyMeasurementClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void GroupsClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void ScenesClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void LevelControlClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void PowerConfigurationClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void AudioOutputClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
};

#endif