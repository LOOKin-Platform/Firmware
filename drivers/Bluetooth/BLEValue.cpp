/*
*    BLEValue.h
*    Bluetooth driver
*
*/

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <esp_log.h>

#include "BLEValue.h"

static char tag[] = "BLEValue";

BLEValue::BLEValue() {
	m_accumulation = "";
	m_value        = "";
	m_readOffset   = 0;
} // BLEValue


/**
 * @brief Add a message part to the accumulation.
 * The accumulation is a growing set of data that is added to until a commit or cancel.
 * @param [in] part A message part being added.
 */
void BLEValue::AddPart(std::string part) {
	ESP_LOGD(tag, ">> addPart: length=%d", part.length());
	m_accumulation += part;
} // addPart


/**
 * @brief Add a message part to the accumulation.
 * The accumulation is a growing set of data that is added to until a commit or cancel.
 * @param [in] pData A message part being added.
 * @param [in] length The number of bytes being added.
 */
void BLEValue::AddPart(uint8_t* pData, size_t length) {
	ESP_LOGD(tag, ">> addPart: length=%d", length);
	m_accumulation += std::string((char*) pData, length);
} // addPart


/**
 * @brief Cancel the current accumulation.
 */
void BLEValue::Cancel() {
	ESP_LOGD(tag, ">> cancel");
	m_accumulation = "";
	m_readOffset   = 0;
} // cancel


/**
 * @brief Commit the current accumulation.
 * When writing a value, we may find that we write it in "parts" meaning that the writes come in in pieces
 * of the overall message.  After the last part has been received, we may perform a commit which means that
 * we now have the complete message and commit the change as a unit.
 */
void BLEValue::Commit() {
	ESP_LOGD(tag, ">> commit");
	// If there is nothing to commit, do nothing.
	if (m_accumulation.length() == 0) return;
	SetValue(m_accumulation);
	m_accumulation = "";
	m_readOffset   = 0;
} // commit


/**
 * @brief Get a pointer to the data.
 * @return A pointer to the data.
 */
uint8_t* BLEValue::GetData() {
	return (uint8_t*) m_value.data();
}


/**
 * @brief Get the length of the data in bytes.
 * @return The length of the data in bytes.
 */
size_t BLEValue::GetLength() {
	return m_value.length();
} // getLength


/**
 * @brief Get the read offset.
 * @return The read offset into the read.
 */
uint16_t BLEValue::GetReadOffset() {
	return m_readOffset;
} // getReadOffset


/**
 * @brief Get the current value.
 */
std::string BLEValue::GetValue() {
	return m_value;
} // getValue


/**
 * @brief Set the read offset
 * @param [in] readOffset The offset into the read.
 */
void BLEValue::SetReadOffset(uint16_t readOffset) {
	m_readOffset = readOffset;
} // setReadOffset


/**
 * @brief Set the current value.
 */
void BLEValue::SetValue(std::string value) {
	m_value = value;
} // setValue


/**
 * @brief Set the current value.
 * @param [in] pData The data for the current value.
 * @param [in] The length of the new current value.
 */
void BLEValue::SetValue(uint8_t* pData, size_t length) {
	m_value = std::string((char*) pData, length);
} // setValue


#endif // CONFIG_BT_ENABLED
