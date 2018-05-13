/*
*    SPIFlash.h
*    Direct access ESP32 memory
*
*/

#ifndef DRIVERS_SPIFLASH_H_
#define DRIVERS_SPIFLASH_H_

using namespace std;

#include <string>
#include <inttypes.h>
#include <math.h>

#include <esp_err.h>
#include <esp_log.h>
#include "esp_spi_flash.h"
#include <esp_attr.h>

#include "../../../src/include/Settings.h"

class SPIFlash {
	public:
		static esp_err_t 	EraseSector(uint32_t);

		static void			EraseRange(uint32_t Start, uint32_t Range);

		static esp_err_t 	Write 	  	(void *		Data, uint32_t Address, size_t Size = 0);
		static esp_err_t 	WriteUint8 	(uint8_t	Data, uint32_t Address);
		static esp_err_t 	WriteUint16	(uint16_t	Data, uint32_t Address);
		static esp_err_t 	WriteUint32 (uint32_t	Data, uint32_t Address);
		static esp_err_t 	WriteUint64 (uint64_t	Data, uint32_t Address);

		static esp_err_t 	Read		(void *		Data, uint32_t Address, size_t Size = 0);
		static uint8_t  	ReadUint8 	(uint32_t	Address);
		static uint16_t  	ReadUint16	(uint32_t	Address);
		static uint32_t  	ReadUint32	(uint32_t	Address);
		static uint32_t  	ReadUint32LE(uint32_t	Address);
		static uint64_t  	ReadUint64	(uint32_t	Address);
};

#endif
