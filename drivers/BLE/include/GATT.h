/*
 * GATT.h
 *
 *  Created on: May 21, 2020
 *      Author: Dmitriy Lukin
 */

#ifndef DRIVERS_BLE_GATT_H_
#define DRIVERS_BLE_GATT_H_

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_att.h"
#include "host/ble_gatt.h"
#include "host/util/util.h"


#include "esp_log.h"

#undef min
#undef max

#include <assert.h>
#include <stdio.h>

#include <vector>
#include <string>

using namespace std;

class GATTService;

class GATT {
	public:
		GATT();

		static void 		Start();
		static void			Stop();

		static void 		SvrRegisterCB(struct ble_gatt_register_ctxt *ctxt, void *arg);

		static ble_uuid_any_t UUIDStruct(string);

		static void			SetServices(ble_gatt_svc_def *);

		static ble_gatt_svc_def	*Services;

	private:
		static void			Init();
		static bool			IsInited;
};

#endif /* DRIVERS_BLE__GATT_H_ */
