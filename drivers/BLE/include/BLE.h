/*
 * BLE.h
 *
 *  Created on: May 21, 2020
 *      Author: Dmitriy Lukin
 */

#ifndef DRIVERS_BLE_H_
#define DRIVERS_BLE_H_

#include "esp_log.h"

#include "esp_bt.h"
#include "nimble/ble.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "modlog/modlog.h"

#include "GATT.h"
#include "FreeRTOSWrapper.h"


#undef min
#undef max

#undef MODLOG_DFLT

#include <string>

using namespace std;

class BLE {

	public:
		static void 	Start();
		static void 	Stop();

		static void 	SetDeviceName(string);

		static void		SetSleep(bool);

		static void		Advertise();
		static void		AdvertiseStop();

		static int		GAPEvent(struct ble_gap_event *event, void *arg);

		static void 	OnReset(int reason);
		static void 	OnSync();

		static bool		IsRunning();

		static int8_t 	GetRSSIForConnection(uint16_t ConnectionHandle);

		static void 	SetPower(esp_power_level_t powerLevel, esp_ble_power_type_t PowerType);

	private:

		static void 	Init();
		static void 	Deinit();

		static void 	HostTask(void *param);

		static void 	SvrRegisterCB(struct ble_gatt_register_ctxt *ctxt, void *arg);

		static string 	DeviceName;
		static bool 	IsInited;
		static uint8_t 	OwnAddrType;

		static bool		IsAdvertiseValue;
		static bool		IsRunningValue;
};

#endif /* DRIVERS_BLE_H_ */

