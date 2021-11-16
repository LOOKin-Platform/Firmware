#include <PartitionAPI.h>
#include "esp_log.h"

esp_err_t PartitionAPI::ErasePartition(string PartitionName) {
    //const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, PartitionName.c_str());
	const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName.c_str());

    if (Partition == NULL) {
    	ESP_LOGE("PartitionAPI", "Partition with name %s didnt finded", PartitionName.c_str());
    	return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI("Partition found", "!");

    /*
    size_t i = 0;

    do {
    	printf("erasing %d\n",i);
    	ESP_ERROR_CHECK(esp_partition_erase_range(Partition, i, ERASE_SIZE));
    	printf("done %d\n",i);
    	i += ERASE_SIZE;
    } while( i < Partition->size);

    printf("done\n");
	*/

    return esp_partition_erase_range(Partition, 0, Partition->size);
}
