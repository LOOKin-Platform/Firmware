/*
*    PowerManagement.cpp
*    Class to control power management features*
*/

#include "PowerManagement.h"
#include "NimBLEDevice.h"
#include "/Users/dmitriy/ESP32/esp-idf/components/esp_coex/include/esp_coexist.h" //! needed to fix

static char tag[] = "PowerManagement";

PowerManagement::PowerManagementType PowerManagement::ActivePMType = NONE;
map<string, PowerManagementLock> PowerManagement::Locks = map<string, PowerManagementLock>();

esp_coex_prefer_t PowerManagement::CurrentPriority = ESP_COEX_PREFER_WIFI;
uint64_t PowerManagement::CurrentPriorityChangeTime = 0;

void PowerManagement::SetPMType(PowerManagementType PMType) {
	if (PMType == ActivePMType)
		return;

	ESP_LOGE("PowerManagement::SetPMType", "%d", PMType);

	ActivePMType = PMType;

	SetWiFiOptions();
	SetBLEOptions();
	SetPMOptions();

	//SetWirelessPriority(ESP_COEX_PREFER_BALANCE);
	SetWirelessPriority(ESP_COEX_PREFER_WIFI);
}


void PowerManagement::SetPMType(bool IsActive, bool IsConstPower) {
	if (IsConstPower)
		SetPMType((IsActive) ? PowerManagement::LIGHT : PowerManagement::NONE);
	else
		SetPMType((IsActive) ? PowerManagement::MAX : PowerManagement::LIGHT);
}

PowerManagement::PowerManagementType PowerManagement::GetPMType() {
	return ActivePMType;
}

void PowerManagement::SetPMOptions() {
	esp_pm_config_t pm_config;

	pm_config.max_freq_mhz 			= 160;
    pm_config.min_freq_mhz 			= (ActivePMType != NONE) ? 40 : 160;
    pm_config.light_sleep_enable 	= (ActivePMType != NONE) ? true : false;

    esp_err_t ret;
    if((ret = esp_pm_configure(&pm_config)) != ESP_OK)
        ESP_LOGI(tag, "pm config error %s", Converter::ErrorToString(ret));
}

void PowerManagement::SetWiFiOptions() {
	switch (ActivePMType)
	{
		case NONE	: esp_wifi_set_ps(WIFI_PS_NONE); break;
		case LIGHT	: esp_wifi_set_ps(WIFI_PS_MIN_MODEM); break;
		case MAX	: esp_wifi_set_ps(WIFI_PS_MAX_MODEM); break;
	}
}

void PowerManagement::SetWirelessPriority(esp_coex_prefer_t Option) {
	if (Option == CurrentPriority && CurrentPriorityChangeTime > 0)
		return;

	ESP_LOGE("Current Wireless priority set", "%d", Option);

	CurrentPriority = Option;

	if (Option == ESP_COEX_PREFER_WIFI)
		esp_coex_status_bit_clear(ESP_COEX_ST_TYPE_BLE, ESP_COEX_BLE_ST_MESH_CONFIG);
	else
		esp_coex_status_bit_set(ESP_COEX_ST_TYPE_BLE, ESP_COEX_BLE_ST_MESH_CONFIG);

	CurrentPriorityChangeTime = Time::UptimeU();
}

esp_coex_prefer_t PowerManagement::GetWirelessPriority() {
	return CurrentPriority;
}

uint64_t PowerManagement::GetWirelessPriorityChangeTime() {
	return CurrentPriorityChangeTime;
}


void PowerManagement::SetBLEOptions() {
	if (!BLEDevice::getInitialized())
		return;

	if (ActivePMType == NONE)
		::esp_bt_sleep_disable();
	else
		::esp_bt_sleep_enable();
}

void PowerManagement::AddLock(string LockName) {
	PowerManagementLock Lock;

	if (ActivePMType == NONE) {
		ESP_LOGD(tag, "PowerManagement not in active state. Can't set lock %s", LockName.c_str());
		return;
	}

	if (Locks.count(LockName) > 0)
	{
		ESP_LOGE(tag, "Lock with name %s has already existed", LockName.c_str());
		return;
	}

	if (esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "RMT_APB_FREQ_MAX", &Lock.APB) == ESP_OK) {
		if (esp_pm_lock_acquire(Lock.APB) != ESP_OK)
			ESP_LOGE(tag, "Error while acquiring APB Lock");
	}
	else
		ESP_LOGE(tag, "Error while creating APB Lock");

	if (esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "RMT_CPU_FREQ_MAX", &Lock.CPU) == ESP_OK) {
		if (esp_pm_lock_acquire(Lock.CPU) != ESP_OK)
			ESP_LOGE(tag, "Error while acquiring CPU Lock");
	}
	else
		ESP_LOGE(tag, "Error while creating CPU Lock");

	if (esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "RMT_NOSLEEP_LOCK", &Lock.Sleep) == ESP_OK) {
		if (esp_pm_lock_acquire(Lock.Sleep) != ESP_OK)
			ESP_LOGE(tag, "Error while acquiring CPU Lock");
	}
	else
		ESP_LOGE(tag, "Error while creating NO SLEEP Lock");

	Locks.insert(pair<string,PowerManagementLock>(LockName, Lock));
}

void PowerManagement::ReleaseLock(string LockName) {
	map<string,PowerManagementLock>::iterator it = Locks.find(LockName);

	  if (it != Locks.end()) {
		esp_pm_lock_release(it->second.APB);
		esp_pm_lock_release(it->second.CPU);
		esp_pm_lock_release(it->second.Sleep);
		Locks.erase(it);
	  }
	  else
		  ESP_LOGE(tag, "Lock with name %s didn't exist", LockName.c_str());
}
