EmberAfStatus HandleReadTempMeasurementAttribute(MatterTempSensor * dev, chip::AttributeId attributeId, uint8_t * buffer, uint16_t maxReadLength)
{
    if ((attributeId == ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID) && (maxReadLength == 2))
    {
        int16_t measuredValue = dev->GetTemperature();
        memcpy(buffer, &measuredValue, sizeof(measuredValue));
    }
    else if ((attributeId == ZCL_TEMP_MIN_MEASURED_VALUE_ATTRIBUTE_ID) && (maxReadLength == 2))
    {
        int16_t minValue = dev->GetMin();
        memcpy(buffer, &minValue, sizeof(minValue));
    }
    else if ((attributeId == ZCL_TEMP_MAX_MEASURED_VALUE_ATTRIBUTE_ID) && (maxReadLength == 2))
    {
        int16_t maxValue = dev->GetMax();
        memcpy(buffer, &maxValue, sizeof(maxValue));
    }
    else if ((attributeId == ZCL_FEATURE_MAP_SERVER_ATTRIBUTE_ID) && (maxReadLength == 4))
    {
        uint32_t featureMap = ZCL_TEMPERATURE_SENSOR_FEATURE_MAP;
        memcpy(buffer, &featureMap, sizeof(featureMap));
    }
    else if ((attributeId == ZCL_CLUSTER_REVISION_SERVER_ATTRIBUTE_ID) && (maxReadLength == 2))
    {
        uint16_t clusterRevision = ZCL_TEMPERATURE_SENSOR_CLUSTER_REVISION;
        memcpy(buffer, &clusterRevision, sizeof(clusterRevision));
    }
    else
    {
        return EMBER_ZCL_STATUS_FAILURE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus HandleReadBridgedDeviceBasicAttribute(MatterGenericDevice * dev, chip::AttributeId attributeId, uint8_t * buffer,
                                                    uint16_t maxReadLength)
{
    ChipLogProgress(DeviceLayer, "HandleReadBridgedDeviceBasicAttribute: attrId=%d, maxReadLength=%d", attributeId, maxReadLength);

    if ((attributeId == ZCL_REACHABLE_ATTRIBUTE_ID) && (maxReadLength == 1))
    {
        *buffer = dev->IsReachable() ? 1 : 0;
    }
    else if ((attributeId == ZCL_NODE_LABEL_ATTRIBUTE_ID) && (maxReadLength == 32))
    {
        MutableByteSpan zclNameSpan(buffer, maxReadLength);
        MakeZclCharString(zclNameSpan, dev->GetName());
    }
    else if ((attributeId == ZCL_CLUSTER_REVISION_SERVER_ATTRIBUTE_ID) && (maxReadLength == 2))
    {
        *buffer = (uint16_t) ZCL_BRIDGED_DEVICE_BASIC_CLUSTER_REVISION;
    }
    else
    {
        return EMBER_ZCL_STATUS_FAILURE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus HandleReadOnOffAttribute(MatterGenericDevice * dev, chip::AttributeId attributeId, uint8_t * buffer, uint16_t maxReadLength)
{
    ChipLogProgress(DeviceLayer, "HandleReadOnOffAttribute: attrId=%d, maxReadLength=%d", attributeId, maxReadLength);

    if ((attributeId == ZCL_ON_OFF_ATTRIBUTE_ID) && (maxReadLength == 1))
    {
        *buffer = (dev)->GetOnOff() ? 1 : 0;
    }
    else if ((attributeId == ZCL_CLUSTER_REVISION_SERVER_ATTRIBUTE_ID) && (maxReadLength == 2))
    {
        *buffer = (uint16_t) ZCL_ON_OFF_CLUSTER_REVISION;
    }
    else
    {
        return EMBER_ZCL_STATUS_FAILURE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}


EmberAfStatus emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                   const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer,
                                                   uint16_t maxReadLength)
{
	ESP_LOGE(Tag, "emberAfExternalAttributeReadCallback");

    uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

    MatterGenericDevice* FindedDevice = Matter::GetDeviceByDynamicIndex(endpointIndex);

    if (FindedDevice != nullptr)
    {
        if (clusterId == BridgedDeviceBasic::Id)
        {
            return HandleReadBridgedDeviceBasicAttribute(FindedDevice, attributeMetadata->attributeId, buffer, maxReadLength);
        }
        else if (clusterId == OnOff::Id)
        {
            return HandleReadOnOffAttribute(FindedDevice, attributeMetadata->attributeId, buffer, maxReadLength);
        }
		else if (clusterId == TemperatureMeasurement::Id)
        {
            return HandleReadTempMeasurementAttribute(static_cast<MatterTempSensor *>(FindedDevice), attributeMetadata->attributeId, buffer, maxReadLength);
        }
    }

    return EMBER_ZCL_STATUS_FAILURE;
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
                                                    const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer)
{
	ESP_LOGE(Tag, "emberAfExternalAttributeWriteCallback");

    uint16_t endpointIndex = emberAfGetDynamicIndexFromEndpoint(endpoint);

	ESP_LOGE("Endpoint INdex", "%d", endpointIndex);

    if (endpointIndex < CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT)
    {
        MatterGenericDevice * dev = Matter::GetDeviceByDynamicIndex(endpointIndex);

        if (dev != nullptr)
            if ((dev->IsReachable()) && (clusterId == OnOff::Id))
				return dev->HandleWriteAttribute(attributeMetadata->attributeId, buffer);
    }

    return EMBER_ZCL_STATUS_FAILURE;
}

bool emberAfActionsClusterInstantActionCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                                const Actions::Commands::InstantAction::DecodableType & commandData)
{
    // No actions are implemented, just return status NotFound.
    commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::NotFound);
    return true;
}