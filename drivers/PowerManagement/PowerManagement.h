/*
*    PowerManagement.h
*    Class to control power management features
*
*/

#ifndef DRIVERS_POWERMANAGEMENT_H_
#define DRIVERS_POWERMANAGEMENT_H_

#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_log.h>

#include "BLE.h"

#include "Log.h"

struct PowerManagementLock {
	esp_pm_lock_handle_t CPU;
	esp_pm_lock_handle_t APB;
	esp_pm_lock_handle_t Sleep;
};

class PowerManagement {
	public:
		static void SetIsActive(bool);
		static bool GetIsActive();

		static void SetPMOptions();
		static void SetWiFiOptions();
		static void SetBLEOptions();

		static void AddLock(string);
		static void ReleaseLock(string);
	private:
		static map<string, PowerManagementLock> Locks;
		static bool IsActive;

};

#endif /* DRIVERS_POWERMANAGEMENT_H_ */
