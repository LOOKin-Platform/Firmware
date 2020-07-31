#include "GATT.h"
#include "Converter.h"

#undef min
#undef max

static char Tag[] = "NimBLEGATT";

bool GATT::IsInited = false;
ble_gatt_svc_def *GATT::Services = NULL;

void GATT::SvrRegisterCB(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}



GATT::GATT() {
}

void GATT::Init() {
    ble_svc_gap_init();
    ble_svc_gatt_init();

	IsInited = true;
}

void GATT::SetServices(ble_gatt_svc_def *sServices) {
	GATT::Services = sServices;
}

void GATT::Setup() {
	if (!IsInited)
		Init();

	ble_gatts_reset();

    int rc = ble_gatts_count_cfg(Services);
    if (rc != 0) {
        ESP_LOGE(Tag,"ble_gatts_count_cfg error %d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(Services);
    if (rc != 0) {
        ESP_LOGE(Tag,"ble_gatts_add_svcs error %d", rc);
        return;
    }
}

void GATT::Start() {
	::ble_gatts_start();
}

void GATT::Stop() {
	::ble_gatts_stop();
}

ble_uuid_any_t IRAM_ATTR GATT::UUIDStruct(string UUID) {
	ble_uuid_any_t Result;

	UUID.erase(std::remove(UUID.begin(), UUID.end(), '-'), UUID.end());

	vector<uint8_t> UUIDVector = vector<uint8_t>();

	while (UUID.length() >= 2) {
		UUIDVector.push_back(Converter::UintFromHexString<uint8_t>(UUID.substr(UUID.length()-2,2)));
		UUID = UUID.substr(0, UUID.length()-2);
	}

	switch (UUIDVector.size()) {
		case 2:
			Result.u.type = 16;
			Result.u16.value = UUIDVector.at(1) * 256 + UUIDVector.at(0);
			break;

		case 4:
			Result.u.type = 32;
			Result.u32.value = UUIDVector.at(3) * 16777216 + UUIDVector.at(2) * 65536 +
								UUIDVector.at(1) * 256 + UUIDVector.at(3);
			break;

		case 16:
			Result.u.type = 128;
			std::copy(UUIDVector.begin(), UUIDVector.end(), Result.u128.value);
			break;

		default:
			ESP_LOGE(Tag, "Error convert to UUID from string, wrong format, UUIDVector size %d", UUIDVector.size());
	}

	return Result;
}



