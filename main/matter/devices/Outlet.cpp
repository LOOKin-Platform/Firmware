#include "Outlet.h"

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;


MatterOutlet::MatterOutlet(string szDeviceName, string szLocation) : MatterGenericDevice(szDeviceName, szLocation) {
    DeviceType = MatterGenericDevice::Light;

    SetReachable(true);
    mChanged_CB = &MatterOutlet::HandleStatusChanged;
};

EmberAfStatus MatterOutlet::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) 
{
    ESP_LOGE("Outlet", "HandleReadOnOffAttribute: attrId=%lu, maxReadLength=%d", AttributeID, maxReadLength);

    if ((AttributeID == OnOff::Attributes::OnOff::Id) && (maxReadLength == 1))
    {
        uint8_t IsOn = GetOnOff() ? 1 : 0;
        *Buffer = IsOn;

        ESP_LOGE("HandleReadAttribute GETONOFF", "%d", (GetOnOff() ? 1 : 0));
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

EmberAfStatus MatterOutlet::HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) 
{
    ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Outlet cluster: clusterID=0x%lx attrId=0x%lx with value %d", ClusterID, AttributeID, *Value);

    if (ClusterID == 0x0006 && AttributeID == 0x0000)
    {
        if(*Value == 1) MatterSendIRCommand("01FF");
        else            MatterSendIRCommand("02FF");

        SetOnOff(*Value == 1);
    }
                
    return EMBER_ZCL_STATUS_SUCCESS;
}

bool MatterOutlet::GetOnOff()
{ 
    ESP_LOGI("Outlet GetOnOff", "Invoked");
    return (mState == kState_On);
}

void MatterOutlet::SetOnOff(bool aOn)
{
    ESP_LOGI("Outlet SetOnOff", "Invoked");

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

void MatterOutlet::HandleStatusChanged(MatterOutlet * dev, MatterOutlet::Changed_t itemChangedMask)
{
    if (itemChangedMask & MatterGenericDevice::kChanged_Reachable)
        ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id);

    if (itemChangedMask & MatterGenericDevice::kChanged_State)
        ScheduleReportingCallback(dev, chip::app::Clusters::OnOff::Id, chip::app::Clusters::OnOff::Attributes::OnOff::Id);

    if (itemChangedMask & MatterGenericDevice::kChanged_Name)
        ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
}

void MatterOutlet::HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask)
{
    if (mChanged_CB)
    {
        mChanged_CB(this, (MatterOutlet::Changed_t) changeMask);
    }
}