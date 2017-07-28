#include "esp_spi_flash.h"
#include "esp_partition.h"

#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"

#include "OTA.h"
#include "Globals.h"

static const char *tag = "OTA";

OTA_t::OTA_t() {};

void OTA_t::Update(string URL) {
  ESP_LOGI(tag, "Starting OTA...");

  OTAHTTPClient_t* HTTPClient = new OTAHTTPClient_t();
  HTTPClient->Port            = OTA_SERVER_PORT;
  HTTPClient->Hostname        = OTA_SERVER_HOST;
  HTTPClient->ContentURI      = URL;

  HTTPClient->Request();
}

void OTAHTTPClient_t::ReadStarted() {
  ESP_LOGD(tag, "ReadStarted");

  BinaryFileLength  = 0;
  OutHandle         = 0;

  bool isInitSucceed = false;

  esp_err_t err;
  const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();

  if (esp_current_partition->type != ESP_PARTITION_TYPE_APP)
    ESP_LOGE(tag, "Error： esp_current_partition->type != ESP_PARTITION_TYPE_APP");

  esp_partition_t find_partition;
  memset(&OperatePartition, 0, sizeof(esp_partition_t));

  // Определение в какую партицию должно быть записано обновление
  switch (esp_current_partition->subtype)
  {
    case ESP_PARTITION_SUBTYPE_APP_OTA_0:
        find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
        break;
    case ESP_PARTITION_SUBTYPE_APP_OTA_1:
        find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    default:
      break;
  }

  find_partition.type = ESP_PARTITION_TYPE_APP;

  const esp_partition_t *partition = esp_partition_find_first(find_partition.type, find_partition.subtype, NULL);
  assert(partition != NULL);
  memset(&OperatePartition, 0, sizeof(esp_partition_t));

  err = esp_ota_begin( partition, OTA_SIZE_UNKNOWN, &OutHandle);
  if (err != ESP_OK) {
    ESP_LOGE(tag, "esp_ota_begin failed err=0x%x!", err);
  }
  else {
      memcpy(&OperatePartition, partition, sizeof(esp_partition_t));
      ESP_LOGI(tag, "esp_ota_begin init OK");
      isInitSucceed = true;
  }

  if (isInitSucceed) {
    ESP_LOGI(tag, "OTA Init succeeded");
  }
  else {
    ESP_LOGE(tag, "OTA Init failed");
  }
}

bool OTAHTTPClient_t::ReadBody(char Data[], int DataLen) {
  esp_err_t err = esp_ota_write(OutHandle, (const void *)Data, DataLen);

  if (err != ESP_OK) {
      ESP_LOGE(tag, "Error: esp_ota_write failed! err=0x%x", err);
      return false;
  }

  BinaryFileLength += DataLen;
  ESP_LOGI(tag, "Have written image length %d", BinaryFileLength);

  return true;
}

bool OTAHTTPClient_t::ReadFinished() {
  ESP_LOGI(tag, "Total Write binary data length : %d", BinaryFileLength);

  if (esp_ota_end(OutHandle) != ESP_OK) {
      ESP_LOGE(tag, "esp_ota_end failed!");
      return false;
  }

  esp_err_t err = esp_ota_set_boot_partition(&OperatePartition);
  if (err != ESP_OK) {
      ESP_LOGE(tag, "esp_ota_set_boot_partition failed! err=0x%x", err);
      return false;
  }

  ESP_LOGI(tag, "Prepare to restart system!");
  esp_restart();

  return true;
}

void OTAHTTPClient_t::Aborted() {
  Device->Status = DeviceStatus::RUNNING;
}
