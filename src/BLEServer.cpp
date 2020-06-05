/*
*    BLEServer.cpp
*    Class for handling Bluetooth connections
*
*/

#include "Globals.h"
#include "BLEServer.h"
#include "Converter.h"
#include "FreeRTOSTask.h"
#include "PowerManagement.h"

static char tag[] = "BLEServer";

static const ble_uuid_any_t DeviceServiceUUID 					= GATT::UUIDStruct("180A");
static const ble_uuid_any_t DeviceServiceManufacturerUUID 		= GATT::UUIDStruct("2A29");
static const ble_uuid_any_t DeviceServiceModelUUID 				= GATT::UUIDStruct("2A24");
static const ble_uuid_any_t DeviceServiceIDUUID 				= GATT::UUIDStruct("2A25");
static const ble_uuid_any_t DeviceServiceFirmwareUUID 			= GATT::UUIDStruct("2A26");
static const ble_uuid_any_t DeviceServiceDeviceHarwareModelUUID	= GATT::UUIDStruct("2A27");
static const ble_uuid_any_t DeviceServiceControlFlagUUID		= GATT::UUIDStruct("3000");
static const ble_uuid_any_t DeviceServiceWiFiUUID 				= GATT::UUIDStruct("4000");
static const ble_uuid_any_t DeviceServiceMQTTUUID 				= GATT::UUIDStruct("5000");

static struct ble_gatt_svc_def Services[2] = {};
static struct ble_gatt_chr_def DeviceServiceCharacteristics[9] = {};

struct ble_gatt_chr_def DeviceManufactorer;
struct ble_gatt_chr_def DeviceModel;
struct ble_gatt_chr_def DeviceID;
struct ble_gatt_chr_def DeviceFirmware;
struct ble_gatt_chr_def DeviceHarwareModel;
struct ble_gatt_chr_def DeviceControlFlag;
struct ble_gatt_chr_def DeviceWiFi;
struct ble_gatt_chr_def DeviceMQTT;

BLEServer_t::BLEServer_t() {
	DeviceManufactorer.uuid 		= &DeviceServiceManufacturerUUID.u;
	DeviceManufactorer.access_cb 	= GATTDeviceManufactorerCallback;
	DeviceManufactorer.flags 		= BLE_GATT_CHR_F_READ;

	DeviceModel.uuid 				= &DeviceServiceModelUUID.u;
	DeviceModel.access_cb 			= GATTDeviceModelCallback;
	DeviceModel.flags 				= BLE_GATT_CHR_F_READ;

	DeviceID.uuid 					= &DeviceServiceIDUUID.u;
	DeviceID.access_cb 				= GATTDeviceIDCallback;
	DeviceID.flags 					= BLE_GATT_CHR_F_READ;

	DeviceFirmware.uuid 			= &DeviceServiceFirmwareUUID.u;
	DeviceFirmware.access_cb 		= GATTDeviceFirmwareCallback;
	DeviceFirmware.flags 			= BLE_GATT_CHR_F_READ;

	DeviceHarwareModel.uuid 		= &DeviceServiceDeviceHarwareModelUUID.u;
	DeviceHarwareModel.access_cb 	= GATTDeviceHardwareModelCallback;
	DeviceHarwareModel.flags 		= BLE_GATT_CHR_F_READ;

	DeviceControlFlag.uuid 			= &DeviceServiceControlFlagUUID.u;
	DeviceControlFlag.access_cb 	= GATTDeviceControlFlagCallback;
	DeviceControlFlag.flags 		= BLE_GATT_CHR_F_WRITE;

	DeviceWiFi.uuid 				= &DeviceServiceWiFiUUID.u;
	DeviceWiFi.access_cb 			= GATTDeviceWiFiCallback;
	DeviceWiFi.flags 				= BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE;

	DeviceMQTT.uuid 				= &DeviceServiceMQTTUUID.u;
	DeviceMQTT.access_cb 			= GATTDeviceMQTTCallback;
	DeviceMQTT.flags 				= BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE;
}

void BLEServer_t::GATTSetup() {
	// Setting up Services and characteristics
	DeviceServiceCharacteristics[0] = DeviceManufactorer;
	DeviceServiceCharacteristics[1] = DeviceModel;
	DeviceServiceCharacteristics[2] = DeviceID;
	DeviceServiceCharacteristics[3] = DeviceFirmware;
	DeviceServiceCharacteristics[4] = DeviceHarwareModel;
	DeviceServiceCharacteristics[5] = DeviceControlFlag;

	if (IsPrivateMode)	DeviceServiceCharacteristics[6] = DeviceWiFi;
	else				DeviceServiceCharacteristics[6] = {0};

	if (IsPrivateMode)	DeviceServiceCharacteristics[7] = DeviceMQTT;
	else				DeviceServiceCharacteristics[7] = {0};

	ble_gatt_svc_def DeviceService;
	DeviceService.type = BLE_GATT_SVC_TYPE_PRIMARY;
	DeviceService.uuid = &DeviceServiceUUID.u;
	DeviceService.characteristics = DeviceServiceCharacteristics;

	Services[0] = DeviceService;
	Services[1] = { 0 };

	GATT::SetServices(Services);
}

static char GATTBuffer[128] = "";

void BLEServer_t::StartAdvertising(string Payload, bool ShouldUsePrivateMode) {
	ESP_LOGI(tag, ">> StartAdvertising");
	GATTSetup();

	if (!IsInited) {
		BLE::Start();
		IsInited = true;
	}
	else {
		BLE::Advertise();
	}

	// FFFF - Sensors and Commands Information
	//pServiceActuators = pServer->CreateService((uint16_t)0xFFFF);

	/*
	for (auto& Sensor : Sensors) {
		BLECharacteristic* pCharacteristicSensor = pServiceActuators->createCharacteristic(BLEUUID((uint16_t)Sensor->ID), BLECharacteristic::PROPERTY_READ);
		pCharacteristicSensor->SetValue(Converter::ToHexString(Sensor->GetValue().Value,2));
	}

	for (auto& Command : Commands) {
		BLECharacteristic* pCharacteristicCommand = pServiceActuators->createCharacteristic(BLEUUID((uint16_t)Command->ID), BLECharacteristic::PROPERTY_WRITE);
		pCharacteristicCommand->setCallbacks(new ServerCallback());
	}
	*/

	ESP_LOGI(tag, "<< StartAdvertising");
}

void BLEServer_t::StopAdvertising() {
	if (!BLE::IsRunning())
		return;

	BLE::AdvertiseStop();
	//	BLE::Stop();

	ESP_LOGI(tag, "BLE Server stopped advertising");
}

void BLEServer_t::SwitchToPublicMode() {
	ESP_LOGI(tag, "SwitchToPublicMode");

	if (BLE::IsRunning() && IsPrivateMode) {
		StopAdvertising();
		FreeRTOS::Sleep(1000);
		StartAdvertising();
	}
}

void BLEServer_t::SwitchToPrivateMode() {
	ESP_LOGI(tag, "SwitchToPrivateMode");

	if (BLE::IsRunning() && !IsPrivateMode) {
		StopAdvertising();
		FreeRTOS::Sleep(1000);
		StartAdvertising("", true);
	}
}

bool BLEServer_t::IsInPrivateMode() {
	return IsPrivateMode;
}

int BLEServer_t::GATTDeviceManufactorerCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
	    return BLE_ATT_ERR_UNLIKELY;

    string Result = "LOOK.in";
    return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int BLEServer_t::GATTDeviceModelCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
	    return BLE_ATT_ERR_UNLIKELY;

    string Result = Converter::ToHexString(Settings.eFuse.Type,2);
    return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int BLEServer_t::GATTDeviceIDCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
	    return BLE_ATT_ERR_UNLIKELY;

    string Result = Converter::ToHexString(Settings.eFuse.DeviceID,8);
    return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int BLEServer_t::GATTDeviceFirmwareCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
	    return BLE_ATT_ERR_UNLIKELY;

    return os_mbuf_append(ctxt->om, Settings.FirmwareVersion.c_str(), Settings.FirmwareVersion.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int BLEServer_t::GATTDeviceHardwareModelCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
	    return BLE_ATT_ERR_UNLIKELY;

    string Result = Converter::ToHexString(Settings.eFuse.Model,2);
    return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int BLEServer_t::GATTDeviceControlFlagCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
	    return BLE_ATT_ERR_UNLIKELY;

    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);

    /* read sent data */
    int rc = ble_hs_mbuf_to_flat(ctxt->om, &GATTBuffer, sizeof GATTBuffer, &om_len);
    string ControlFlag(GATTBuffer, om_len);

	if (ControlFlag.size() > 0) {
		if (ControlFlag == "1") {
			ESP_LOGI(tag, "Received signal to switch on WiFi for interval");
			Network.WiFiConnect();
		}
	}

    return (rc == 0) ? 0 : BLE_ATT_ERR_UNLIKELY;
}

int BLEServer_t::GATTDeviceWiFiCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
	    string Result = Converter::VectorToString(Network.GetSavedWiFiList(), ",");
	    return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
	    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);

	    /* read sent data */
	    int rc = ble_hs_mbuf_to_flat(ctxt->om, &GATTBuffer, sizeof GATTBuffer, &om_len);
	    string WiFiData(GATTBuffer, om_len);

		if (WiFiData.length() > 0) {
			JSON JSONItem(WiFiData);

			map<string,string> Params;

			ESP_LOGE(tag, "Value %s", WiFiData.c_str());

			if (JSONItem.GetType() == JSON::RootType::Object)
				Params = JSONItem.GetItems();

			if (!(Params.count("s") && Params.count("p")))
			{
				vector<string> Parts = Converter::StringToVector(WiFiData," ");
				Params["s"] = Parts[0].c_str();
				Params["p"] = Parts[1].c_str();
			}

			if (!(Params.count("s") && Params.count("p")))
			{
				ESP_LOGE(tag, "WiFi characteristics error input format");
			}
			else
			{
				ESP_LOGD(tag, "WiFi Data received. SSID: %s, Password: %s", Params["s"].c_str(), Params["p"].c_str());
				Network.AddWiFiNetwork(Params["s"], Params["p"]);

				//If device in AP mode or can't connect as Station - try to connect with new data
				if ((WiFi.GetMode() == WIFI_MODE_STA_STR && WiFi.GetConnectionStatus() == UINT8_MAX) || (WiFi.GetMode() == WIFI_MODE_AP_STR))
					Network.WiFiConnect(Params["s"], true);
			}
		}

	    return (rc == 0) ? 0 : BLE_ATT_ERR_UNLIKELY;
	}

    return BLE_ATT_ERR_UNLIKELY;
}

int BLEServer_t::GATTDeviceMQTTCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
	    string Result = MQTT.GetClientID();
	    return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
	    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);

	    int rc = ble_hs_mbuf_to_flat(ctxt->om, &GATTBuffer, sizeof GATTBuffer, &om_len);
	    string MQTTData(GATTBuffer, om_len);

		if (MQTTData.length() > 0) {
			vector<string> Parts = Converter::StringToVector(MQTTData," ");

			if (Parts.size() != 2) {
				ESP_LOGE(tag, "MQTT credentials error input format");
			}
			else
			{
				ESP_LOGD(tag, "MQTT credentials data received. ClientID: %s, ClientSecret: %s", Parts[0].c_str(), Parts[1].c_str());
				MQTT.ChangeOrSetCredentialsBLE(Parts[0], Parts[1]);
			}
		}

	    return (rc == 0) ? 0 : BLE_ATT_ERR_UNLIKELY;
	}

    return BLE_ATT_ERR_UNLIKELY;}


