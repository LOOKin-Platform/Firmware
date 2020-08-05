/*
*    SPIFlash.cpp
*    Direct access ESP32 memory
*
*/

#include "SPIFlash.h"

const char tag[] = "SPIFlash";

/**
 * Erase sector of memory.
 * @param[in]  Sector address. Note that sector address must be 0x1000 aligned
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::EraseSector(uint32_t Sector) {
	return spi_flash_erase_sector(Sector);
}

/**
 * Erase range between 2 addresses
 * @param[in]  Start address. Note that sector address must be 0x4 aligned
 * @param[in]  Length of erased area. Note that length must be 0x4 aligned
 *
 */
void IRAM_ATTR SPIFlash::EraseRange(uint32_t Start, uint32_t Length) {
    if (Start + Length > spi_flash_get_chip_size())
        return;

    if (Length % 4 != 0) {
    	ESP_LOGE(tag,"Range must be 4 bytes aligned");
    	return;
    }

    if (Start % 4 != 0) {
    	ESP_LOGE(tag,"Start must be 4 bytes aligned");
    	return;
    }

    uint32_t	BlockStart 		= (Start / Settings.Memory.BlockSize) * Settings.Memory.BlockSize; // block starts every 4kb
    uint8_t		BlocksToErase 	= ceil((Start + Length) / Settings.Memory.BlockSize) - BlockStart / Settings.Memory.BlockSize + 1;

    uint32_t 	*HeadBuffer 	= (uint32_t *) malloc((Start - BlockStart));
    uint32_t 	*TailBuffer 	= (uint32_t *) malloc((BlockStart + BlocksToErase*Settings.Memory.BlockSize - Start - Length));

	::spi_flash_read(BlockStart		, HeadBuffer, (Start - BlockStart));
    ::spi_flash_read(Start + Length	, TailBuffer, (BlockStart + BlocksToErase*Settings.Memory.BlockSize - Start - Length));

    ESP_LOGE("SPI FLASH", "BlockStart: %d, BlocksToErase: %d", BlockStart, BlocksToErase);
    ESP_LOGE("SPI FLASH", "Start address: %d, Size: %d", BlockStart, (Start - BlockStart));
    ESP_LOGE("SPI FLASH", "End address: %d, Size: %d", Start + Length, (BlockStart + BlocksToErase*Settings.Memory.BlockSize - Start - Length));

    for (int i=0; i < BlocksToErase; i++)
    		EraseSector((BlockStart + i*Settings.Memory.BlockSize) / Settings.Memory.BlockSize);

	::spi_flash_write(BlockStart	, HeadBuffer, (Start - BlockStart));
    ::spi_flash_write(Start + Length, TailBuffer, (BlockStart + BlocksToErase*Settings.Memory.BlockSize - Start - Length));

    free(HeadBuffer);
    free(TailBuffer);
}

/**
 * Write data to SPI Flash
 * @param[in]  Data to write
 * @param[in]  Address to write data
 * @param[in]  Size Data size
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::Write(void* Data, uint32_t Address, size_t Size) {
	if (Size == 0)
		Size = sizeof(Data);

    if ((Address + Size) > spi_flash_get_chip_size())
    	return ESP_ERR_INVALID_SIZE;
    else
    	return ::spi_flash_write(Address, Data, Size);
}

/**
 * Write uint8_t data to SPI Flash
 * @param[in]  Data to write
 * @param[in]  Address to write data
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::WriteUint8(uint8_t Data, uint32_t Address) {
	return Write((void*)(&Data), Address, 1);
//	return ::spi_flash_write(Address, (void*)(&Data), sizeof(Data));
}

/**
 * Write uint16_t data to SPI Flash
 * @param[in]  Data to write
 * @param[in]  Address to write data
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::WriteUint16(uint16_t Data, uint32_t Address) {
	return Write((void*)(&Data), Address, 2);
}

/**
 * Write uint32_t data to SPI Flash
 * @param[in]  Data to write
 * @param[in]  Address to write data
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::WriteUint32(uint32_t Data, uint32_t Address) {
	return Write((void*)(&Data), Address, 4);
}

/**
 * Write uint64_t data to SPI Flash
 * @param[in]  Data to write
 * @param[in]  Address to write data
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::WriteUint64(uint64_t Data, uint32_t Address) {
	esp_err_t Err = WriteUint32((uint32_t)((Data << 32) >> 32), Address);

	if (Err == ESP_OK)
		Err = WriteUint32((uint32_t)(Data >> 32), Address + 0x04);

	return Err;
}

/**
 * Read data from SPI Flash
 * @param[in]  Data Reference to data to read
 * @param[in]  Address to read data
 * @param[in]  Size Data size
 * @param[out] Result of operation
 *
 */
esp_err_t IRAM_ATTR SPIFlash::Read(void *Data, uint32_t Address, size_t Size) {
	if (Size == 0)
		Size = sizeof(Data);

	if (Size == 0)
		return ESP_ERR_INVALID_SIZE;

	return ::spi_flash_read(Address, Data, Size);
}

/**
 * Read uint8_t from SPI Flash
 * @param[in]  Address to read data
 * @param[out] uint8_t data
 *
 */
uint8_t IRAM_ATTR SPIFlash::ReadUint8(uint32_t Address) {
    uint8_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 1)) ? Result : 0;
}

/**
 * Read uint16_t from SPI Flash
 * @param[in]  Address to read data
 * @param[out] uint16_t data
 *
 */
uint16_t IRAM_ATTR SPIFlash::ReadUint16(uint32_t Address) {
    uint16_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 2)) ? Result : 0;
}

/**
 * Read uint32_t from SPI Flash
 * @param[in]  Address to read data
 * @param[out] uint32_t data
 *
 */
uint32_t IRAM_ATTR SPIFlash::ReadUint32(uint32_t Address) {
    uint32_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 4)) ? Result : 0;
}


/**
 * Read uint32_t from SPI Flash in Little Endian
 * @param[in]  Address to read data
 * @param[out] uint32_t data
 *
 */
uint32_t IRAM_ATTR SPIFlash::ReadUint32LE(uint32_t Address) {
    uint32_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 4)) ? __builtin_bswap32(Result) : 0;
}

/**
 * Read uint64_t from SPI Flash
 * @param[in]  Address to read data
 * @param[out] uint64_t data
 *
 */
uint64_t IRAM_ATTR SPIFlash::ReadUint64(uint32_t Address) {
    uint64_t Result = 0;

    Result += ReadUint32(Address + 0x4);
    Result  = Result << 32;
    Result += ReadUint32(Address);

    return Result;
}
