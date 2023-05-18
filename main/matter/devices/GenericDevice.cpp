#include "GenericDevice.h"
#include "Matter.h"


MatterGenericDevice::MatterGenericDevice(string szDeviceName, string szLocation)
{
    CopyString(mName, sizeof(mName), szDeviceName.c_str());
    CopyString(mLocation, sizeof(mLocation), szLocation.c_str());

    mReachable  = false;
    mEndpointId = 0xFFFF;

    //dataVersions.reserve(3);
}

bool MatterGenericDevice::IsReachable() const {
    return mReachable;
}

void MatterGenericDevice::SetReachable(bool aReachable) {
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

    if (changed)
        HandleDeviceChange(this, kChanged_Reachable);
}

void MatterGenericDevice::SetName(const char * szDeviceName) {
    bool changed = (strncmp(mName, szDeviceName, sizeof(mName)) != 0);

    ChipLogProgress(DeviceLayer, "Device[%s]: New Name=\"%s\"", mName, szDeviceName);

    CopyString(mName, sizeof(mName), szDeviceName);

    if (changed)
        HandleDeviceChange(this, kChanged_Name);
}

void MatterGenericDevice::SetLocation(const char * szLocation) {
    bool changed = (strncmp(mLocation, szLocation, sizeof(mLocation)) != 0);

    CopyString(mLocation, sizeof(mLocation), szLocation);

    ChipLogProgress(DeviceLayer, "Device[%s]: Location=\"%s\"", mName, mLocation);

    if (changed)
        HandleDeviceChange(this, kChanged_Location);
}

void MatterGenericDevice::HandleDeviceStatusChanged(MatterGenericDevice * dev, MatterGenericDevice::Changed_t itemChangedMask)
{
    ESP_LOGE("MatterDevice", "HandleDeviceStatusChanged");

    if (itemChangedMask & MatterGenericDevice::kChanged_Reachable)
        ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters:: BridgedDeviceBasicInformation::Attributes::Reachable::Id);

    if (itemChangedMask & MatterGenericDevice::kChanged_Name)
        ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
}

void MatterGenericDevice::ScheduleReportingCallback(MatterGenericDevice * dev, chip::ClusterId cluster, chip::AttributeId attribute)
{
    auto * path = chip::Platform::New<chip::app::ConcreteAttributePath>(dev->GetEndpointID(), cluster, attribute);
    chip::DeviceLayer::PlatformMgr().ScheduleWork(CallReportingCallback, reinterpret_cast<intptr_t>(path));
}

void MatterGenericDevice::CallReportingCallback(intptr_t closure) {
    auto path = reinterpret_cast<chip::app::ConcreteAttributePath *>(closure);
    MatterReportingAttributeChangeCallback(*path);
    chip::Platform::Delete(path);
}


/// <summary>
/// Send IR Command to device
/// </summary>
void MatterGenericDevice::MatterSendIRCommand(string operand)
{
    ESP_LOGI("MatterGenericDevice", "MatterSendIRCommand");
    Matter::SendIRWrapper(BridgedUUID, operand);
}


