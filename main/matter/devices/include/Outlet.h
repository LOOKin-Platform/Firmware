#ifndef MATTER_DEVICES_OUTLET
#define MATTER_DEVICES_OUTLET

#include "GenericDevice.h"

#include <cstdio>
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

class MatterOutlet : public MatterGenericDevice {
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

        using DeviceCallback_fn = std::function<void(MatterOutlet *, MatterOutlet::Changed_t)>;

        MatterOutlet(string szDeviceName, string szLocation = "");

        Protocols::InteractionModel::Status HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) override;
        Protocols::InteractionModel::Status HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) override;

        bool GetOnOff() override;
        void SetOnOff(bool aOn) override;

        static void HandleStatusChanged(MatterOutlet * dev, MatterOutlet::Changed_t itemChangedMask);

    private:
        State_t mState = kState_Off;

        DeviceCallback_fn mChanged_CB;

        void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) override;

        void OnOffClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void IdentifyClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
        void PowerConfigClusterHandler(chip::AttributeId AttributeID, uint8_t * Value);
};

#endif