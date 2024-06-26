/*
*    I2C.h
*    Class to control I2C devices
*
*/

#ifndef DRIVERS_I2C_H_
#define DRIVERS_I2C_H_

#include <stdint.h>
#include <sys/types.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <stdint.h>
#include <esp_log.h>

#include "Converter.h"

class I2C {
	public:
		static const 	gpio_num_t 	DEFAULT_SDA_PIN = GPIO_NUM_18;
		static const 	gpio_num_t	DEFAULT_CLK_PIN = GPIO_NUM_19;
		static const 	uint32_t	DEFAULT_CLK_SPEED = 1000000;

		I2C();
		void 			Init(uint8_t address, gpio_num_t sdaPin = DEFAULT_SDA_PIN, gpio_num_t sclPin = DEFAULT_CLK_PIN, uint32_t clkSpeed = DEFAULT_CLK_SPEED, i2c_port_t portNum = I2C_NUM_0);
		void 			Start();
		void 			Stop();
		void 			Scan();

		uint8_t 		GetAddress() const;
		void 			SetAddress(uint8_t address);
		void 			SetDebug(bool enabled);
		bool 			SlavePresent(uint8_t address);

		void 			BeginTransaction();
		void 			EndTransaction();

		void 			Read(uint8_t* bytes, size_t length, bool ack=true);
		void 			Read(uint8_t* byte, bool ack=true);

		void 			Write(uint8_t byte, bool ack=true);
		void 			Write(uint8_t* bytes, size_t length, bool ack=true);

	private:
		uint8_t          m_address			= 0;
		i2c_cmd_handle_t m_cmd				= 0;
		bool             m_directionKnown 	= false;
		gpio_num_t       m_sdaPin			= DEFAULT_SDA_PIN;
		gpio_num_t       m_sclPin			= DEFAULT_CLK_PIN;
		i2c_port_t       m_portNum			= I2C_NUM_0;

};

#endif /* DRIVERS_I2C_H_ */
