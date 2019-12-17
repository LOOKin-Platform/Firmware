/*
*    BLE2902.cpp
*    Bluetooth driver
*
*/

/*
 * See also:
 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.characteristic_presentation_format.xml
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLE2904.h"

BLE2904::BLE2904() : BLEDescriptor(BLEUUID((uint16_t) 0x2904)) {
	m_data.m_format      = 0;
	m_data.m_exponent    = 0;
	m_data.m_namespace   = 1;  // 1 = Bluetooth SIG Assigned Numbers
	m_data.m_unit        = 0;
	m_data.m_description = 0;
	SetValue((uint8_t*) &m_data, sizeof(m_data));
} // BLE2902


/**
 * @brief Set the description.
 */
void BLE2904::SetDescription(uint16_t description) {
	m_data.m_description = description;
	SetValue((uint8_t*) &m_data, sizeof(m_data));
}


/**
 * @brief Set the exponent.
 */
void BLE2904::SetExponent(int8_t exponent) {
	m_data.m_exponent = exponent;
	SetValue((uint8_t*) &m_data, sizeof(m_data));
} // setExponent


/**
 * @brief Set the format.
 */
void BLE2904::SetFormat(uint8_t format) {
	m_data.m_format = format;
	SetValue((uint8_t*) &m_data, sizeof(m_data));
} // setFormat


/**
 * @brief Set the namespace.
 */
void BLE2904::SetNamespace(uint8_t namespace_value) {
	m_data.m_namespace = namespace_value;
	SetValue((uint8_t*) &m_data, sizeof(m_data));
} // setNamespace


/**
 * @brief Set the units for this value.  It should be one of the encoded values defined here:
 * https://www.bluetooth.com/specifications/assigned-numbers/units
 * @param [in] unit The type of units of this characteristic as defined by assigned numbers.
 */
void BLE2904::SetUnit(uint16_t unit) {
	m_data.m_unit = unit;
	SetValue((uint8_t*) &m_data, sizeof(m_data));
} // setUnit

#endif
