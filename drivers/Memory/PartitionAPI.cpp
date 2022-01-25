#include <PartitionAPI.h>
#include "esp_log.h"

esp_err_t PartitionAPI::ErasePartition(string PartitionName, uint8_t PartitionType) {
    //const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, PartitionName.c_str());
	const esp_partition_t *Partition = esp_partition_find_first((esp_partition_type_t)PartitionType, ESP_PARTITION_SUBTYPE_ANY, PartitionName.c_str());

    if (Partition == NULL) {
    	ESP_LOGE("PartitionAPI", "Partition with name %s not found", PartitionName.c_str());
    	return ESP_ERR_NOT_FOUND;
    }

    return esp_partition_erase_range(Partition, 0, Partition->size);
}
