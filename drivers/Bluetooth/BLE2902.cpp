/*
*    BLE2902.cpp
*    Bluetooth driver
*
*/

/*
 * See also:
 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLE2902.h"

BLE2902::BLE2902() : BLEDescriptor(BLEUUID((uint16_t) 0x2902)) {
	uint8_t data[2] = { 0, 0 };
	SetValue(data, 2);
} // BLE2902


/**
 * @brief Get the notifications value.
 * @return The notifications value.  True if notifications are enabled and false if not.
 */
bool BLE2902::GetNotifications() {
	return (GetValue()[0] & (1 << 0)) != 0;
} // getNotifications


/**
 * @brief Get the indications value.
 * @return The indications value.  True if indications are enabled and false if not.
 */
bool BLE2902::GetIndications() {
	return (GetValue()[0] & (1 << 1)) != 0;
} // getIndications


/**
 * @brief Set the indications flag.
 * @param [in] flag The indications flag.
 */
void BLE2902::SetIndications(bool flag) {
	uint8_t *pValue = GetValue();
	if (flag) pValue[0] |= 1 << 1;
	else pValue[0] &= ~(1 << 1);
} // setIndications


/**
 * @brief Set the notifications flag.
 * @param [in] flag The notifications flag.
 */
void BLE2902::SetNotifications(bool flag) {
	uint8_t *pValue = GetValue();
	if (flag) pValue[0] |= 1 << 0;
	else pValue[0] &= ~(1 << 0);
} // setNotifications

#endif
