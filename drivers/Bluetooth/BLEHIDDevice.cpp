/*
 *    BLEHIDDevice.cpp
 *    Bluetooth driver
 *
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLEHIDDevice.h"
#include "BLE2904.h"

BLEHIDDevice::BLEHIDDevice(BLEServerGeneric* server) {
	/*
	 * Here we create mandatory services described in bluetooth specification
	 */
	m_deviceInfoService = server->CreateService(BLEUUID((uint16_t) 0x180a));
	m_hidService = server->CreateService(BLEUUID((uint16_t) 0x1812), 40);
	m_batteryService = server->CreateService(BLEUUID((uint16_t) 0x180f));

	/*
	 * Mandatory characteristic for device info service
	 */
	m_pnpCharacteristic = m_deviceInfoService->CreateCharacteristic((uint16_t) 0x2a50, BLECharacteristic::PROPERTY_READ);

	/*
	 * Mandatory characteristics for HID service
	 */
	m_hidInfoCharacteristic 		= m_hidService->CreateCharacteristic((uint16_t) 0x2a4a, BLECharacteristic::PROPERTY_READ);
	m_reportMapCharacteristic 		= m_hidService->CreateCharacteristic((uint16_t) 0x2a4b, BLECharacteristic::PROPERTY_READ);
	m_hidControlCharacteristic 		= m_hidService->CreateCharacteristic((uint16_t) 0x2a4c, BLECharacteristic::PROPERTY_WRITE_NR);
	m_protocolModeCharacteristic 	= m_hidService->CreateCharacteristic((uint16_t) 0x2a4e, BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_READ);

	/*
	 * Mandatory battery level characteristic with notification and presence descriptor
	 */
	BLE2904* batteryLevelDescriptor = new BLE2904();
	batteryLevelDescriptor->SetFormat(BLE2904::FORMAT_UINT8);
	batteryLevelDescriptor->SetNamespace(1);
	batteryLevelDescriptor->SetUnit(0x27ad);

	m_batteryLevelCharacteristic = m_batteryService->CreateCharacteristic((uint16_t) 0x2a19, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
	m_batteryLevelCharacteristic->AddDescriptor(batteryLevelDescriptor);
	m_batteryLevelCharacteristic->AddDescriptor(new BLE2902());

	/*
	 * This value is setup here because its default value in most usage cases, its very rare to use boot mode
	 * and we want to simplify library using as much as possible
	 */
	const uint8_t pMode[] = { 0x01 };
	ProtocolMode()->SetValue((uint8_t*) pMode, 1);
}

BLEHIDDevice::~BLEHIDDevice() {
}

/*
 * @brief
 */
void BLEHIDDevice::ReportMap(uint8_t* map, uint16_t size) {
	m_reportMapCharacteristic->SetValue(map, size);
}

/*
 * @brief This function suppose to be called at the end, when we have created all characteristics we need to build HID service
 */
void BLEHIDDevice::StartServices() {
	m_deviceInfoService->Start();
	m_hidService->Start();
	m_batteryService->Start();
}

/*
 * @brief Create manufacturer characteristic (this characteristic is optional)
 */
BLECharacteristic* BLEHIDDevice::Manufacturer() {
	m_manufacturerCharacteristic = m_deviceInfoService->CreateCharacteristic((uint16_t) 0x2a29, BLECharacteristic::PROPERTY_READ);
	return m_manufacturerCharacteristic;
}

/*
 * @brief Set manufacturer name
 * @param [in] name manufacturer name
 */
void BLEHIDDevice::Manufacturer(std::string name) {
	m_manufacturerCharacteristic->SetValue(name);
}

/*
 * @brief
 */
void BLEHIDDevice::Pnp(uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version) {
	uint8_t pnp[] = { sig, (uint8_t) (vid >> 8), (uint8_t) vid, (uint8_t) (pid >> 8), (uint8_t) pid, (uint8_t) (version >> 8), (uint8_t) version };
	m_pnpCharacteristic->SetValue(pnp, sizeof(pnp));
}

/*
 * @brief
 */
void BLEHIDDevice::HidInfo(uint8_t country, uint8_t flags) {
	uint8_t info[] = { 0x11, 0x1, country, flags };
	m_hidInfoCharacteristic->SetValue(info, sizeof(info));
}

/*
 * @brief Create input report characteristic that need to be saved as new characteristic object so can be further used
 * @param [in] reportID input report ID, the same as in report map for input object related to created characteristic
 * @return pointer to new input report characteristic
 */
BLECharacteristic* BLEHIDDevice::InputReport(uint8_t reportID) {
	BLECharacteristic* inputReportCharacteristic = m_hidService->CreateCharacteristic((uint16_t) 0x2a4d, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
	BLEDescriptor* inputReportDescriptor = new BLEDescriptor(BLEUUID((uint16_t) 0x2908));
	BLE2902* p2902 = new BLE2902();
	inputReportCharacteristic->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
	inputReportDescriptor->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
	p2902->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

	uint8_t desc1_val[] = { reportID, 0x01 };
	inputReportDescriptor->SetValue((uint8_t*) desc1_val, 2);
	inputReportCharacteristic->AddDescriptor(p2902);
	inputReportCharacteristic->AddDescriptor(inputReportDescriptor);

	return inputReportCharacteristic;
}

/*
 * @brief Create output report characteristic that need to be saved as new characteristic object so can be further used
 * @param [in] reportID Output report ID, the same as in report map for output object related to created characteristic
 * @return Pointer to new output report characteristic
 */
BLECharacteristic* BLEHIDDevice::OutputReport(uint8_t reportID) {
	BLECharacteristic* outputReportCharacteristic = m_hidService->CreateCharacteristic((uint16_t) 0x2a4d, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
	BLEDescriptor* outputReportDescriptor = new BLEDescriptor(BLEUUID((uint16_t) 0x2908));
	outputReportCharacteristic->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
	outputReportDescriptor->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

	uint8_t desc1_val[] = { reportID, 0x02 };
	outputReportDescriptor->SetValue((uint8_t*) desc1_val, 2);
	outputReportCharacteristic->AddDescriptor(outputReportDescriptor);

	return outputReportCharacteristic;
}

/*
 * @brief Create feature report characteristic that need to be saved as new characteristic object so can be further used
 * @param [in] reportID Feature report ID, the same as in report map for feature object related to created characteristic
 * @return Pointer to new feature report characteristic
 */
BLECharacteristic* BLEHIDDevice::FeatureReport(uint8_t reportID) {
	BLECharacteristic* featureReportCharacteristic = m_hidService->CreateCharacteristic((uint16_t) 0x2a4d, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor* featureReportDescriptor = new BLEDescriptor(BLEUUID((uint16_t) 0x2908));

	featureReportCharacteristic->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
	featureReportDescriptor->SetAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

	uint8_t desc1_val[] = { reportID, 0x03 };
	featureReportDescriptor->SetValue((uint8_t*) desc1_val, 2);
	featureReportCharacteristic->AddDescriptor(featureReportDescriptor);

	return featureReportCharacteristic;
}

/*
 * @brief
 */
BLECharacteristic* BLEHIDDevice::BootInput() {
	BLECharacteristic* bootInputCharacteristic = m_hidService->CreateCharacteristic((uint16_t) 0x2a22, BLECharacteristic::PROPERTY_NOTIFY);
	bootInputCharacteristic->AddDescriptor(new BLE2902());

	return bootInputCharacteristic;
}

/*
 * @brief
 */
BLECharacteristic* BLEHIDDevice::BootOutput() {
	return m_hidService->CreateCharacteristic((uint16_t) 0x2a32, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
}

/*
 * @brief
 */
BLECharacteristic* BLEHIDDevice::HidControl() {
	return m_hidControlCharacteristic;
}

/*
 * @brief
 */
BLECharacteristic* BLEHIDDevice::ProtocolMode() {
	return m_protocolModeCharacteristic;
}

void BLEHIDDevice::SetBatteryLevel(uint8_t level) {
	m_batteryLevelCharacteristic->SetValue(&level, 1);
	m_batteryLevelCharacteristic->Notify();
}
/*
 * @brief Returns battery level characteristic
 * @ return battery level characteristic
 *//*
BLECharacteristic* BLEHIDDevice::batteryLevel() {
	return m_batteryLevelCharacteristic;
}
BLECharacteristic*	 BLEHIDDevice::reportMap() {
	return m_reportMapCharacteristic;
}
BLECharacteristic*	 BLEHIDDevice::pnp() {
	return m_pnpCharacteristic;
}
BLECharacteristic*	BLEHIDDevice::hidInfo() {
	return m_hidInfoCharacteristic;
}
*/
/*
 * @brief
 */
BLEService* BLEHIDDevice::DeviceInfo() {
	return m_deviceInfoService;
}

/*
 * @brief
 */
BLEService* BLEHIDDevice::HidService() {
	return m_hidService;
}

/*
 * @brief
 */
BLEService* BLEHIDDevice::BatteryService() {
	return m_batteryService;
}

#endif // CONFIG_BT_ENABLED
