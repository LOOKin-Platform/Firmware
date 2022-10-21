/*
*    OTA.cpp
*    Over the Air updates implementation
*
*/

#include "OTA.h"
#include "Settings.h"
#include "HTTPClient.h"
#include "esp_https_ota.h"

#include "Log.h"
#include "PowerManagement.h"
#include "Device.h"

extern Device_t				Device;

static char tag[] = "OTA";

int              			OTA::BinaryFileLength  			= 0;
esp_ota_handle_t 			OTA::OutHandle         			= 0;
esp_partition_t  			OTA::OperatePartition;
uint8_t						OTA::Attempts					= 0;
bool						OTA::IsOTAFileExist				= false;
bool						OTA::IsFileCheckEnded			= false;

FreeRTOS::Timer*			OTA::DelayedRebootTimer			= nullptr;

OTAFailed					OTA::OTAFailedCallback			= NULL;

OTA::OTA() {
	BinaryFileLength  = 0;
	OutHandle         = 0;
};

void OTA::Update(string URL, OTAStarted StartedCallback, OTAFailed FailedCallback) {
	Attempts 				= 0;

	IsOTAFileExist			= false;
	IsFileCheckEnded		= false;

	if (StartedCallback != NULL)
		StartedCallback();

	OTAFailedCallback		= FailedCallback;

	PerformUpdate(URL);
}

esp_err_t OTA::PerformUpdate(string URL) {
	ESP_LOGI(tag, "Starting OTA from URL %s", URL.c_str());

	esp_http_client_config_t Config;
	::memset(&Config, 0, sizeof(Config));

	Config.auth_type 					= HTTP_AUTH_TYPE_NONE;

	Config.path							= "/";
	Config.host							= NULL;
	Config.query						= NULL;
	Config.username						= NULL;
	Config.password 					= NULL;

	Config.port							= 80;
	Config.url 							= URL.c_str();

	Config.cert_pem						= NULL;
	Config.client_cert_pem				= NULL;
	Config.client_cert_len				= 0;
	Config.client_key_pem				= NULL;
	Config.client_key_len				= 0;
	Config.client_key_password			= NULL;
	Config.client_key_password_len		= 0;

	Config.use_global_ca_store			= false;
	Config.crt_bundle_attach			= NULL;

	Config.transport_type 				= HTTP_TRANSPORT_UNKNOWN;

	Config.timeout_ms 					= 10000;
	Config.buffer_size 					= Settings.OTA.BufferSize;
	Config.buffer_size_tx 				= Settings.OTA.BufferSize;

	Config.disable_auto_redirect		= false;
	Config.max_redirection_count 		= 10;

	Config.method 						= esp_http_client_method_t::HTTP_METHOD_GET;

	Config.user_data 					= NULL;
	Config.event_handler				= NULL;
	Config.user_agent 					= "";

	Config.is_async						= false;

	Config.skip_cert_common_name_check 	= false;

    Config.keep_alive_enable 			= true;
    Config.keep_alive_idle				= 5;
    Config.keep_alive_interval			= 5;
    Config.keep_alive_count				= 3;

    Config.if_name 						= NULL;

	PowerManagement::AddLock("OTA");

	esp_err_t ret = esp_https_ota(&Config);

    if (ret == ESP_OK)
    {
    	ESP_LOGE(tag, "OTA succeed");
    	Log::Add(Log::Events::System::OTASucceed);

        esp_restart();
    }
    else
    {
    	ESP_LOGE(tag, "OTA failed, %d, %s", ret, Converter::ErrorToString(ret));

		PowerManagement::ReleaseLock("OTA");

		Attempts++;

		if (Attempts < Settings.OTA.MaxAttempts)
			OTA::PerformUpdate(URL);
		else
		{
			Device.Status = DeviceStatus::RUNNING;

			Log::Add((ret == ESP_ERR_OTA_VALIDATE_FAILED) ? Log::Events::System::OTAVerifyFailed : Log::Events::System::OTAFailed);

			if (OTAFailedCallback != NULL)
				OTAFailedCallback();
		}

        return ESP_FAIL;
    }

    return ESP_OK;
}

bool OTA::Rollback() {
	esp_partition_t RollbackPartition;

	const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();
	if (esp_current_partition->type != ESP_PARTITION_TYPE_APP) {
		ESP_LOGE(tag, "Error： esp_current_partition->type != ESP_PARTITION_TYPE_APP");
		return false;
	}

	const esp_partition_t *partition = NULL;
	partition = esp_ota_get_next_update_partition(esp_current_partition);
	assert(partition != NULL);

	memcpy(&RollbackPartition, partition, sizeof(esp_partition_t));

	esp_err_t err = esp_ota_set_boot_partition(&RollbackPartition);

	if (err == ESP_OK)
	{
		ESP_LOGE(tag, "Prepare to restart");

		DelayedRebootTimer = new FreeRTOS::Timer((char*)"DelayedReboot", 50 / portTICK_PERIOD_MS, pdFALSE, NULL, OTA::DelayedRebootTask);
		DelayedRebootTimer->Start();
		return true;
	}
	else {
		ESP_LOGE(tag, "Error occured during partition rollback");
		return false;
	}
}

void OTA::DelayedRebootTask(FreeRTOS::Timer *) {
	esp_restart();
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
	else
	{
		memcpy(&OperatePartition, partition, sizeof(esp_partition_t));
		ESP_LOGD(tag, "esp_ota_begin init OK");
		isInitSucceed = true;
	}

	if (isInitSucceed) 	{
		ESP_LOGI(tag, "OTA Init succeeded");
		Device.Status = DeviceStatus::UPDATING;
	}
	else {
		ESP_LOGE(tag, "OTA Init failed");
		PowerManagement::ReleaseLock("OTA");
		Device.Status = DeviceStatus::RUNNING;
	}
}

bool OTA::ReadBody(char Data[], int DataLen, char IP[]) {
	esp_err_t err = esp_ota_write(OutHandle, (const void *)Data, DataLen);

	if (err != ESP_OK) {
		ESP_LOGE(tag, "Error: esp_ota_write failed! err=0x%x", err);
		Device.Status = DeviceStatus::RUNNING;
		return false;
	}

	BinaryFileLength += DataLen;
	ESP_LOGD(tag, "Have written image length %d", BinaryFileLength);

	return true;
}

void OTA::ReadFinished(char IP[]) {
	ESP_LOGI(tag, "Total Write binary data length : %d", BinaryFileLength);

	esp_err_t err;
	err = esp_ota_end(OutHandle);

	if (err != ESP_OK) {
		if (err == ESP_ERR_OTA_VALIDATE_FAILED)
			Log::Add(Log::Events::System::OTAVerifyFailed);
		else
			Log::Add(Log::Events::System::OTAFailed);

		ESP_LOGE(tag, "esp_ota_end failed!");
		Device.Status = DeviceStatus::RUNNING;
		PowerManagement::ReleaseLock("OTA");

		return;
	}

	err = esp_ota_set_boot_partition(&OperatePartition);
	if (err != ESP_OK) {
		ESP_LOGE(tag, "esp_ota_set_boot_partition failed! err=0x%x", err);
		return;
	}

	Log::Add(Log::Events::System::OTASucceed);

	ESP_LOGD(tag, "Prepare to restart system!");
	esp_restart();
}

void OTA::Aborted(char IP[]) {
	Device.Status = DeviceStatus::RUNNING;
	PowerManagement::ReleaseLock("OTA");
}
