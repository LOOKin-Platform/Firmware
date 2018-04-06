/*
*    SPIFlash.cpp
*    Direct access ESP32 memory
*
*/

#include "SPIFlash.h"

static char tag[] = "SPIFlash";

esp_err_t SPIFlash::EraseSector(uint32_t Sector) {
	return spi_flash_erase_sector(Sector);
}

void SPIFlash::EraseRange(uint32_t Start, uint32_t Range) {
    if (Start + Range > spi_flash_get_chip_size())
        return;

    if (Range % 4 != 0) {
    		ESP_LOGE(tag,"Range must be 4 bytes aligned");
    		return;
    }

    if (Start % 4 != 0) {
    		ESP_LOGE(tag,"Start must be 4 bytes aligned");
    		return;
    }

    uint32_t	BlockStart = (Start / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE; // block starts every 4kb
    uint8_t	BlocksToErase = ceil((Start + Range) / MEMORY_BLOCK_SIZE) - BlockStart / MEMORY_BLOCK_SIZE + 1;

    uint32_t HeadBuffer[(Start - BlockStart) / 4];
    uint32_t TailBuffer[(BlockStart + BlocksToErase*MEMORY_BLOCK_SIZE - Start - Range) / 4];

	::spi_flash_read(BlockStart		, HeadBuffer, sizeof(HeadBuffer));
    ::spi_flash_read(Start + Range	, TailBuffer, sizeof(TailBuffer));

    for (int i=0; i < BlocksToErase; i++)
    		EraseSector((BlockStart + i*MEMORY_BLOCK_SIZE) / MEMORY_BLOCK_SIZE);

	::spi_flash_write(BlockStart		, HeadBuffer, sizeof(HeadBuffer));
    ::spi_flash_write(Start + Range	, TailBuffer, sizeof(TailBuffer));
}


esp_err_t SPIFlash::Write(void* Data, uint32_t Address, size_t Size) {
	if (Size == 0)
		Size = sizeof(Data);

    if (Address + Size > spi_flash_get_chip_size())
        return ESP_ERR_INVALID_SIZE;

	return ::spi_flash_write(Address, Data, Size);
}

esp_err_t SPIFlash::WriteUint8(uint8_t Data, uint32_t Address) {
	return Write((void*)(&Data), Address, 1);
//	return ::spi_flash_write(Address, (void*)(&Data), sizeof(Data));
}

esp_err_t SPIFlash::WriteUint16(uint16_t Data, uint32_t Address) {
	return Write((void*)(&Data), Address, 2);
}

esp_err_t SPIFlash::WriteUint32(uint32_t Data, uint32_t Address) {
	return Write((void*)(&Data), Address, 4);
}

esp_err_t SPIFlash::WriteUint64(uint64_t Data, uint32_t Address) {
	esp_err_t Err = WriteUint32((uint32_t)((Data << 32) >> 32), Address);

	if (Err == ESP_OK)
		Err = WriteUint32((uint32_t)(Data >> 32), Address + 0x04);

	return Err;
}

esp_err_t SPIFlash::Read(void *Data, uint32_t Address, size_t Size) {
	if (Size == 0)
		Size = sizeof(Data);

	if (Size == 0)
		return ESP_ERR_INVALID_SIZE;

	return ::spi_flash_read(Address, Data, Size);
}

uint8_t SPIFlash::ReadUint8(uint32_t Address) {
    uint8_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 1)) ? Result : 0;
}

uint16_t SPIFlash::ReadUint16(uint32_t Address) {
    uint16_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 2)) ? Result : 0;
}

uint32_t SPIFlash::ReadUint32(uint32_t Address) {
    uint32_t Result = 0;
	return (ESP_OK == Read((void *)(&Result), Address, 4)) ? Result : 0;
}

uint64_t SPIFlash::ReadUint64(uint32_t Address) {
    uint64_t Result = 0;

    Result += ReadUint32(Address + 0x4);
    Result = Result << 32;
    Result += ReadUint32(Address);

    return Result;
}
