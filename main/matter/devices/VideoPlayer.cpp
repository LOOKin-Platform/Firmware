#include "VideoPlayer.h"

#define Tag "MatterVideoPlayer"


MatterVideoPlayer:: MatterVideoPlayer(string szDeviceName, string szLocation):MatterGenericDevice(szDeviceName, szLocation)
{
    DeviceType = DeviceTypeEnum::MediaPlayer;

    SetReachable(true);
    mChanged_CB = &MatterVideoPlayer::HandleStatusChanged;
}

void MatterVideoPlayer::HandleStatusChanged(MatterVideoPlayer * dev, MatterVideoPlayer::Changed_t itemChangedMask)
{
    ESP_LOGE("MatterVideoPlayer", "MatterVideoPlayer");
}

EmberAfStatus MatterVideoPlayer::HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength)
{
    ESP_LOGE("MatterVideoPlayer", "HandleReadAttribute, ClusterID: %lu, attributeID: 0x%lx, MaxReadLength: %d", ClusterID, AttributeID, maxReadLength);

    if (ClusterID == chip::app::Clusters::OnOff::Id)
    {
        ESP_LOGE("OnOff handler", "handler");

        if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
        {
            *Buffer = (uint16_t) ZCL_ON_OFF_CLUSTER_REVISION;
        }
        else if ((AttributeID == OnOff::Attributes::OnOff::Id) && (maxReadLength == 1))
        {
            *Buffer = 0;
        }
        else
        {
            return EMBER_ZCL_STATUS_FAILURE; 
        }
    }
    
    if (ClusterID == chip::app::Clusters::KeypadInput::Id)
    {
        ESP_LOGE("KeypadInput", "handler");

        if ((AttributeID == ClusterRevision::Id) && (maxReadLength == 2))
        {
            *Buffer = (uint16_t) KEYPAD_CLUSTER_REVISION;
        }
        else if ((AttributeID == FeatureMap::Id) && (maxReadLength == 4))
        {
            uint32_t FeatureMap = KEYPAD_FEATURE_MAP;
            memcpy(Buffer, &FeatureMap, sizeof(FeatureMap));
        }
        else
        {
            return EMBER_ZCL_STATUS_FAILURE; 
        }
    }

    ESP_LOGE("VideoPlayer", "EMBER_ZCL_STATUS_SUCCESS");
    return EMBER_ZCL_STATUS_SUCCESS;
}

void MatterVideoPlayer::HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask)
{
    ESP_LOGE("MatterVideoPlayer", "HandleDeviceChange");

    if (mChanged_CB)
    {
        mChanged_CB(this, (MatterVideoPlayer::Changed_t) changeMask);
    }
}





void MatterVideoPlayer::OnOffClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "OnOffClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::BasicClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "BasicClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::IdentifyMeasurementClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "IdentifyMeasurementClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::GroupsClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "GroupsClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::ScenesClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "ScenesClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::PowerConfigurationClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "PowerConfigurationClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::LevelControlClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "LevelControlClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}

void MatterVideoPlayer::AudioOutputClusterHandler(chip::AttributeId AttributeID, uint8_t * Value)
{
    ESP_LOGI(Tag, "AudioOutputClusterHandler, AttributeID: %lu", AttributeID);
    if(AttributeID == 0x0000)   //
    {

    }
    else
    {
        ESP_LOGE(Tag, "Wrong AttributeID: %lu", AttributeID);
    }
}