/*
 *    BLEValue.h
 *    Bluetooth driver
 *
 */

#ifndef DRIVERS_BLEVALUE_H_
#define DRIVERS_BLEVALUE_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <string>

/**
 * @brief The model of a %BLE value.
 */
/**
 * @brief The model of a %BLE value.
 */
class BLEValue {
	public:
		BLEValue();
		void		AddPart(std::string part);
		void		AddPart(uint8_t* pData, size_t length);
		void		Cancel();
		void		Commit();
		uint8_t*	GetData();
		size_t	  	GetLength();
		uint16_t	GetReadOffset();
		std::string GetValue();
		void        SetReadOffset(uint16_t readOffset);
		void        SetValue(std::string value);
		void        SetValue(uint8_t* pData, size_t length);

	private:
		std::string m_accumulation;
		uint16_t    m_readOffset;
		std::string m_value;

};
#endif // CONFIG_BT_ENABLED
#endif /* COMPONENTS_CPP_UTILS_BLEVALUE_H_ */
