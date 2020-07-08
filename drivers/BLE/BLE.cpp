
#include "BLE.h"

bool 	BLE::IsInited 			= false;
string 	BLE::DeviceName 		= "";
bool 	BLE::IsRunningValue 	= "";
bool	BLE::IsAdvertiseValue	= false;

static char Tag[] = "BLE";

uint8_t BLE::OwnAddrType = 0;

extern "C" {
	void ble_store_config_init(void);
}

void BLE::Init() {
	int ret = ::esp_nimble_hci_and_controller_init();
	if (ret != ESP_OK) {
	     ESP_LOGE(Tag, "esp_nimble_hci_and_controller_init() failed with error: %d", ret);
	     return;
	}

	::nimble_port_init();

	IsInited = true;
}

void BLE::Deinit() {
	::nimble_port_deinit();

    int ret = esp_nimble_hci_and_controller_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(Tag, "esp_nimble_hci_and_controller_deinit() failed with error: %d", ret);
    }

	IsInited = false;
}

void BLE::SetPower(esp_power_level_t powerLevel, esp_ble_power_type_t PowerType) {
	return;

	ESP_LOGD(Tag, ">> setPower: %d", powerLevel);

	esp_err_t errRc = ::esp_ble_tx_power_set(PowerType, powerLevel);
	if (errRc != ESP_OK)
		ESP_LOGE(Tag, "esp_ble_tx_power_set: rc=%d", powerLevel);

	ESP_LOGD(Tag, "<< setPower");
}

/*
void BLE::SetPowerLevel(int Power) {
	::ble_phy_txpwr_set(Level);
}
*/

void BLE::SetSleep(bool SleepEnabled) {
	if (SleepEnabled)
		::esp_bt_sleep_enable();
	else
		::esp_bt_sleep_disable();
}



/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
void BLE::Advertise() {
	ESP_LOGE(Tag,"Advertise()");

    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    ble_uuid16_t Services[1] = { GATT::UUIDStruct("120A").u16 };
    fields.uuids16 = Services;

    fields.num_uuids16 			= 1;
    fields.uuids16_is_complete 	= 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode 	= BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode 	= BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min		= 500;
    adv_params.itvl_max		= 1000;

    rc = ble_gap_adv_start(OwnAddrType, NULL, BLE_HS_FOREVER, &adv_params, BLE::GAPEvent, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }

    IsAdvertiseValue = true;
}

void BLE::AdvertiseStop() {
    IsAdvertiseValue = false;
	::ble_gap_adv_stop();
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */

int BLE::GAPEvent(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type)
    {
    	case BLE_GAP_EVENT_CONNECT:
    		/* A new connection was established or a connection attempt failed. */
    		MODLOG_DFLT(INFO, "connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
    		if (event->connect.status == 0) {
    			rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
    			assert(rc == 0);
    		}
    		MODLOG_DFLT(INFO, "\n");

    		if (event->connect.status != 0) {
    			/* Connection failed; resume advertising. */
    			Advertise();
    		}
    		return 0;

    	case BLE_GAP_EVENT_DISCONNECT:
    		MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
    		MODLOG_DFLT(INFO, "\n");

    		/* Connection terminated; resume advertising. */
    		Advertise();
    		return 0;

    	case BLE_GAP_EVENT_CONN_UPDATE:
    		/* The central has updated the connection parameters. */
    		MODLOG_DFLT(INFO, "connection updated; status=%d ",
                    event->conn_update.status);
    		rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
    		assert(rc == 0);
        	MODLOG_DFLT(INFO, "\n");
        	return 0;

    	case BLE_GAP_EVENT_ADV_COMPLETE:
    		MODLOG_DFLT(INFO, "advertise complete; reason=%d", event->adv_complete.reason);
    		Advertise();
    		return 0;

    	case BLE_GAP_EVENT_SUBSCRIBE:
    		MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
    		return 0;

    	case BLE_GAP_EVENT_MTU:
    		MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
    		return 0;
    }

    return 0;
}

void BLE::OnReset(int reason) {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

void BLE::OnSync(void) {
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &OwnAddrType);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(OwnAddrType, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    MODLOG_DFLT(INFO, "\n");
    /* Begin advertising. */
    Advertise();
}

bool BLE::IsRunning() {
	return IsRunningValue;
}

void BLE::Start() {
	if (!IsInited) Init();

	if (!IsInited) return;

    /* Initialize the NimBLE host configuration. */
	ble_hs_cfg.reset_cb 			= BLE::OnReset;
	ble_hs_cfg.sync_cb 				= BLE::OnSync;
	ble_hs_cfg.gatts_register_cb	= BLE::SvrRegisterCB;
	ble_hs_cfg.store_status_cb 		= ble_store_util_status_rr;

	ble_hs_cfg.sm_io_cap 			= 3;	// BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_mitm 				= 0; 	// MITM security = false
    ble_hs_cfg.sm_sc 				= 0; 	// Enable/disable Security Manager Secure Connection 4.2 feature.

    GATT::Start();

	int rc = ble_svc_gap_device_name_set(DeviceName.c_str());

	if (rc != 0) {
		ESP_LOGE(Tag, "ble_svc_gap_device_name_set error %d", rc);
		return;
	}

	ble_store_config_init();

	nimble_port_freertos_init(BLE::HostTask);
	if (rc != 0) {
		ESP_LOGE(Tag, "nimble_port_freertos_init error %d", rc);
		return;
	}
}

void BLE::Stop() {
	::ble_gap_adv_stop();
	::nimble_port_stop();
}

void BLE::SetDeviceName(string Name) {
	if (DeviceName == Name) return;

	DeviceName = Name;
}

void BLE::HostTask(void *param)
{
	IsRunningValue = true;
    ESP_LOGI(Tag, "BLE Host Task Started");
    ::nimble_port_run();

    ::nimble_port_freertos_deinit();
    ESP_LOGI(Tag, "BLE Host Task Stopped");
    IsRunningValue = false;
}

void BLE::SvrRegisterCB(struct ble_gatt_register_ctxt *ctxt, void *arg) {
	GATT::SvrRegisterCB(ctxt, arg);
}

