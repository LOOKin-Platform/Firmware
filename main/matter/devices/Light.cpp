#include "Light.h"

#define Tag "MatterLight"

using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::Globals::Attributes;

MatterLight::MatterLight(string szDeviceName, string szLocation) : MatterGenericDevice(szDeviceName, szLocation)
{
    DeviceType = MatterGenericDevice::Light;

    SetReachable(true);
    mChanged_CB = &MatterLight::HandleStatusChanged;
}

EmberAfStatus MatterLight::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength) 
{
    ESP_LOGE("Light", "HandleReadOnOffAttribute: attrId=%lu, maxReadLength=%d", AttributeID, maxReadLength);

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

EmberAfStatus MatterLight::HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value) 
{
    ChipLogProgress(DeviceLayer, "HandleWriteAttribute for Light cluster: clusterID=%lu attrId=0x%lx", ClusterID, AttributeID);

    ReturnErrorCodeIf((AttributeID != OnOff::Attributes::OnOff::Id) || (!IsReachable()), EMBER_ZCL_STATUS_FAILURE);


    ESP_LOGE(">>>>>>>>>>>>>>>>>>>>>>>MatterLight", "AttributeID: %lu, ClusterId: %lu", AttributeID, ClusterID);

    if (ClusterID == 0x0006)
    {
        OnOffClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0008)
    {
        LevelControlClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0300)
    {
        ColorControlClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0005)
    {
        ScenesClusterHandler(AttributeID, Value);
    }
    else if(ClusterID == 0x0004)
    {
        GroupsClusterHandler(AttributeID, Value);
    }
    else
    {
        ESP_LOGE(Tag, "Wrong ClusterID: %lu", ClusterID);
    }

    SetOnOff(*Value == 1);
    return EMBER_ZCL_STATUS_SUCCESS;
}

bool MatterLight::GetOnOff()
{ 
    ESP_LOGE("Light device GetOnOff", "Invoked"); 
    return (mState == kState_On);
}

void MatterLight::SetOnOff(bool aOn) 
{
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

void MatterLight::HandleStatusChanged(MatterLight * dev, MatterLight::Changed_t itemChangedMask)
{
    if (itemChangedMask & MatterGenericDevice::kChanged_Reachable)
        ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id);

    if (itemChangedMask & MatterGenericDevice::kChanged_State)
        ScheduleReportingCallback(dev, chip::app::Clusters::OnOff::Id, chip::app::Clusters::OnOff::Attributes::OnOff::Id);

    if (itemChangedMask & MatterGenericDevice::kChanged_Name)
        ScheduleReportingCallback(dev, chip::app::Clusters::BridgedDeviceBasicInformation::Id, chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id);
}

void MatterLight::HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask)
{
    if (mChanged_CB)
    {
        mChanged_CB(this, (MatterLight::Changed_t) changeMask);
    }
}

void MatterLight::OnOffClusterHandler(chip::AttributeId AttributeID, uint8_t *Value) {
    ESP_LOGI(Tag, "OnOffClusterHandler, AttributeID: %lu", AttributeID);
    if (AttributeID == 0x0000)  //On/Off attribute
    {
        if(*Value == 1) MatterSendIRCommand("01FF");
        else            MatterSendIRCommand("02FF");
        SetOnOff(*Value == 1);
    }
    else if(AttributeID == 0x4000)  //GlobalSceneControl
    {

    }
    else if(AttributeID == 0x4001)  //OnTime
    {

    }
    else if(AttributeID == 0x4002)  //OffWaitTime
    {

    }
    else if(AttributeID == 0x4003)  //StartUpOnOff
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterLight::LevelControlClusterHandler(chip::AttributeId AttributeID, uint8_t *Value) {
    ESP_LOGI(Tag, "LevelControlClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //CurrentLevel
    {

    }
    else if(AttributeID == 0x0001)  //RemainingTime
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterLight::ColorControlClusterHandler(chip::AttributeId AttributeID, uint8_t *Value) {
    ESP_LOGI(Tag, "ColorControlClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //CurrentHue
    {

    }
    else if(AttributeID == 0x0001)  //CurrentSaturation
    {

    }
    else if(AttributeID == 0x0002)  //ColorMode
    {

    }
    else if(AttributeID == 0x0003)  //Options
    {

    }
    else if(AttributeID == 0x0010)  //NumberOfPrimaries
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterLight::GroupsClusterHandler(chip::AttributeId AttributeID, uint8_t *Value) {
    ESP_LOGI(Tag, "GroupsClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //NameSupport
    {

    }
    else if(AttributeID == 0x0001)  //GroupID
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterLight::ScenesClusterHandler(chip::AttributeId AttributeID, uint8_t *Value) {
    ESP_LOGI(Tag, "ScenesClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //SceneCount
    {

    }
    else if(AttributeID == 0x0001)  //CurrentScene
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}
