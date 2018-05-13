/*
*    OTA.cpp
*    Over the Air updates implementation
*
*/

#include "OTA.h"
#include "Globals.h"

static char tag[] = "OTA";

int              OTA::BinaryFileLength  = 0;
esp_ota_handle_t OTA::OutHandle         = 0;
esp_partition_t  OTA::OperatePartition;

OTA::OTA() {
	BinaryFileLength  = 0;
	OutHandle         = 0;
};

void OTA::Update(string URL) {
	ESP_LOGI(tag, "Starting OTA...");

	HTTPClient::Query(Settings.OTA.ServerHost, Settings.OTA.ServerPort, URL, QueryType::GET, "",
                        true, &ReadStarted, &ReadBody, &ReadFinished, &Aborted);
}

void OTA::ReadStarted(char IP[]) {
	ESP_LOGD(tag, "ReadStarted");

	bool isInitSucceed = false;
	BinaryFileLength = 0;

	esp_err_t err;
	const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();

	if (esp_current_partition->type != ESP_PARTITION_TYPE_APP)
		ESP_LOGE(tag, "Error： esp_current_partition->type != ESP_PARTITION_TYPE_APP");

	const esp_partition_t *partition = NULL;
	partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(tag, "Writing to partition subtype %d at offset 0x%x", partition->subtype, partition->address);
    assert(partition != NULL);


	err = esp_ota_begin( partition, OTA_SIZE_UNKNOWN, &OutHandle);
	if (err != ESP_OK) {
		ESP_LOGE(tag, "esp_ota_begin failed err=0x%x!", err);
	}
	else {
		memcpy(&OperatePartition, partition, sizeof(esp_partition_t));
		ESP_LOGI(tag, "esp_ota_begin init OK");
		isInitSucceed = true;
	}

	if (isInitSucceed) 	{	ESP_LOGI(tag, "OTA Init succeeded");}
	else 				{	ESP_LOGE(tag, "OTA Init failed");	}
}

bool OTA::ReadBody(char Data[], int DataLen, char IP[]) {
	esp_err_t err = esp_ota_write(OutHandle, (const void *)Data, DataLen);

	if (err != ESP_OK) {
		ESP_LOGE(tag, "Error: esp_ota_write failed! err=0x%x", err);
		return false;
	}

	BinaryFileLength += DataLen;
	ESP_LOGI(tag, "Have written image length %d", BinaryFileLength);

	return true;
}

void OTA::ReadFinished(char IP[]) {
	ESP_LOGI(tag, "Total Write binary data length : %d", BinaryFileLength);

	if (esp_ota_end(OutHandle) != ESP_OK) {
		ESP_LOGE(tag, "esp_ota_end failed!");
		return;
	}

	esp_err_t err = esp_ota_set_boot_partition(&OperatePartition);
	if (err != ESP_OK) {
		ESP_LOGE(tag, "esp_ota_set_boot_partition failed! err=0x%x", err);
		return;
	}

	ESP_LOGI(tag, "Prepare to restart system!");
	esp_restart();
}

void OTA::Aborted(char IP[]) {
	Device.Status = DeviceStatus::RUNNING;
}

void OTA::Rollback() {
	esp_partition_t RollbackPartition;

	const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();
	if (esp_current_partition->type != ESP_PARTITION_TYPE_APP) {
		ESP_LOGE(tag, "Can't find partition to rollback");
		return;
	}

	esp_partition_t find_partition;
	memset(&RollbackPartition, 0, sizeof(esp_partition_t));

	// На какую партицию должен быть осуществлен сброс
	switch (esp_current_partition->subtype) {
		case ESP_PARTITION_SUBTYPE_APP_OTA_0:
			find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
    		break;
    case ESP_PARTITION_SUBTYPE_APP_OTA_1:
    		find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
    		break;
    default:
    		find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_FACTORY;
    		break;
	}

	find_partition.type = ESP_PARTITION_TYPE_APP;

	const esp_partition_t *partition = esp_partition_find_first(find_partition.type, find_partition.subtype, NULL);
	assert(partition != NULL);
	memset(&RollbackPartition, 0, sizeof(esp_partition_t));


	esp_err_t err = esp_ota_set_boot_partition(&RollbackPartition);

	if (err == ESP_OK)
		esp_restart();
}
