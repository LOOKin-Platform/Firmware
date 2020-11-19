#include <PartitionAPI.h>

esp_err_t PartitionAPI::ErasePartition(string PartitionName) {
    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName.c_str());

    if (Partition == NULL)
    	return ESP_ERR_NOT_FOUND;

    return esp_partition_erase_range(Partition, 0, Partition->size);
}
