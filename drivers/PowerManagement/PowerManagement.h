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
#include "esp_coexist.h"

#include "Log.h"

struct PowerManagementLock {
	esp_pm_lock_handle_t CPU;
	esp_pm_lock_handle_t APB;
	esp_pm_lock_handle_t Sleep;
};

class PowerManagement {
	public:
		enum PowerManagementType { NONE, LIGHT, MAX };

		static void SetPMType(PowerManagementType);
		static void SetPMType(bool IsActive, bool IsConstPower);
		static PowerManagementType GetPMType();

		static void SetPMOptions();
		static void SetWiFiOptions();
		static void SetBLEOptions();

		static void AddLock(string);
		static void ReleaseLock(string);

		static void SetWirelessPriority(esp_coex_prefer_t Option);

	private:
		static map<string, PowerManagementLock> Locks;
		static PowerManagementType ActivePMType;

};

#endif /* DRIVERS_POWERMANAGEMENT_H_ */
