/*
*    Network.cpp
*    Class to handle API /Device
*
*/
#include "Globals.h"
#include "Data.h"

const char *DataEndpoint_t::Tag 			= "Data_t";
string		DataEndpoint_t::NVSArea			= "Data";

DataEndpoint_t* DataEndpoint_t::GetForDevice() {
	switch (Settings.eFuse.Type)
	{
		case 0x81	: return new DataRemote_t();
		default		: return new DataEndpoint_t();
	}
}

pair<uint32_t, uint32_t> DataEndpoint_t::SplitAddres(uint64_t FullAddress) {
	 return make_pair((uint32_t)(FullAddress >> 32),(uint32_t)((FullAddress << 32) >> 32));
}

uint64_t DataEndpoint_t::JoinAddress(uint32_t Start, uint32_t Size) {
    uint64_t NVSPointerToItem = (uint64_t)Start;
    return (uint64_t)((NVSPointerToItem << 32) + Size);
}

uint32_t DataEndpoint_t::NormalizedItemAddress(uint32_t Address) {
	if ((Address % MinRecordSize) == 0)
		return Address;

	return ((Address / MinRecordSize) + 1) * MinRecordSize;
}

uint32_t DataEndpoint_t::GetFreeMemoryAddress() {
	NVS Memory(DataEndpoint_t::NVSArea);
	return Memory.GetUInt32Bit(FREE_MEMORY_NVS);
}

void DataEndpoint_t::SetFreeMemoryAddress(uint32_t Address) {
	NVS Memory(DataEndpoint_t::NVSArea);
	Memory.SetUInt32Bit(FREE_MEMORY_NVS, NormalizedItemAddress(Address));
	Memory.Commit();
}


void DataEndpoint_t::Defragment() {
	nvs_handle handle;
	nvs_open(DataEndpoint_t::NVSArea.c_str(), NVS_READWRITE, &handle);

	vector<tuple<uint32_t, uint32_t, string>> MemoryMap = vector<tuple<uint32_t,uint32_t, string>>();

	nvs_iterator_t it = ::nvs_entry_find(NVS_DEFAULT_PART_NAME, DataEndpoint_t::NVSArea.c_str(), NVS_TYPE_ANY);
	while (it != NULL)
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		it = nvs_entry_next(it);

		uint64_t AddressAndSize;
		esp_err_t nvs_err = nvs_get_u64(handle, info.key, &AddressAndSize);

		if (nvs_err == ESP_OK)
		{
			uint32_t Address 	= (uint32_t)(AddressAndSize >> 32);
			uint32_t Size		= (uint32_t)((AddressAndSize << 32) >> 32);

			if (Size > 0)
				MemoryMap.push_back(make_tuple(Address, Size, string(info.key)));
			else
				nvs_erase_key(handle, info.key);
		}
	}

    if (MemoryMap.size() == 0) return;

    sort(MemoryMap.begin(), MemoryMap.end());

    for (uint16_t i = 1; i < MemoryMap.size(); i++) {
    	uint32_t PreviousEnd = NormalizedItemAddress(get<0>(MemoryMap[i-1]) + get<1>(MemoryMap[i-1]));

    	if (get<0>(MemoryMap[i]) > PreviousEnd) {
    		Move(PreviousEnd, get<0>(MemoryMap[i]), get<1>(MemoryMap[i]));
    		get<0>(MemoryMap[i]) = PreviousEnd;

    	    uint64_t NVSPointerToItem = (uint64_t)PreviousEnd;
    	    NVSPointerToItem = (uint64_t)((NVSPointerToItem << 32) + get<1>(MemoryMap[i]));
    		nvs_set_u64(handle, get<2>(MemoryMap[i]).c_str(), NVSPointerToItem);
    	}
    }

	nvs_close(handle);
}

void DataEndpoint_t::Move(uint32_t NewAddress, uint32_t OldAddress, uint32_t Size) {
	uint32_t Counter = 0;

    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);
    if (Partition == NULL) return;

	static char ReadBuffer[MemoryBlockSize - MinRecordSize];
	memset(ReadBuffer, 0, MemoryBlockSize - MinRecordSize);

	while (Counter < Size)
	{
		uint16_t BufferSize = MemoryBlockSize - MinRecordSize;

		// Clear memory where to write new block
		uint32_t 	BlockStart 			= floor(((NewAddress + Counter) / MemoryBlockSize)) * MemoryBlockSize;
		bool 		IsWriteInBlockStart = (NewAddress == BlockStart);

	    EraseRange(NewAddress + Counter, MemoryBlockSize - MinRecordSize);

		::esp_partition_read		(Partition, OldAddress + Counter, ReadBuffer, MemoryBlockSize - MinRecordSize);
	    ::esp_partition_write		(Partition, NewAddress + Counter, &ReadBuffer, BufferSize);

	    Counter += MemoryBlockSize - MinRecordSize;
	}
}

void IRAM_ATTR DataEndpoint_t::EraseRange(uint32_t Start, uint32_t Length) {
    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);
    if (Partition == NULL) return;

    if (Start + Length > spi_flash_get_chip_size())
        return;

    if (Length % 4 != 0 || Start % 4 != 0) {
    	ESP_LOGE(Tag, "Length and Start must be 4 bytes aligned");
    	return;
    }

    uint32_t	BlockStart 		= (Start / MemoryBlockSize) * MemoryBlockSize; // block starts every 4kb
    uint8_t		BlocksToErase 	= ceil((Start + Length) / (float)MemoryBlockSize) - BlockStart / MemoryBlockSize;

    uint32_t	*HeadBuffer 	= (uint32_t *) calloc((Start - BlockStart), sizeof(uint32_t));
    uint32_t	*TailBuffer 	= (uint32_t *) calloc((BlockStart + BlocksToErase*MemoryBlockSize - Start - Length), sizeof(uint32_t));

	::esp_partition_read(Partition, BlockStart		, HeadBuffer, (Start - BlockStart));
    ::esp_partition_read(Partition, Start + Length	, TailBuffer, (BlockStart + BlocksToErase*MemoryBlockSize - Start - Length));

    /*
    ESP_LOGE(Tag, "EraseRange() In data: Start: %d, Length: %d", Start, Length);
    ESP_LOGE(Tag, "EraseRange() BlockStart: %d, BlocksToErase: %d", BlockStart, BlocksToErase);
    ESP_LOGE(Tag, "EraseRange() Start address: %d, Size: %d", BlockStart, (Start - BlockStart));
    ESP_LOGE(Tag, "EraseRange() End address: %d, Size: %d", Start + Length, (BlockStart + BlocksToErase*MemoryBlockSize - Start - Length));
    */

    for (int i=0; i < BlocksToErase; i++)
    	::esp_partition_erase_range(Partition, (BlockStart + i*MemoryBlockSize) / MemoryBlockSize, BlockStart + BlocksToErase*MemoryBlockSize);

    if ((Start - BlockStart) > 0)
    	::esp_partition_write(Partition, BlockStart	, HeadBuffer, (Start - BlockStart));

    if ((BlockStart + BlocksToErase*MemoryBlockSize - Start - Length) > 0)
    	::esp_partition_write(Partition, Start + Length, TailBuffer, (BlockStart + BlocksToErase*MemoryBlockSize - Start - Length));

    free(HeadBuffer);
    free(TailBuffer);
}

bool DataEndpoint_t::SaveItem(string ItemName, string Item) {
	NVS Memory(DataEndpoint_t::NVSArea);
#if (CONFIG_FIRMWARE_TARGET_SIZE_4MB)
	Memory.SetString(ItemName, Item);
	return true;
#else
	if (ItemName == FREE_MEMORY_NVS)
		return false;

    uint32_t ReplacementAddress = 0xFFFFFFFF;
	uint64_t ExistedAddress = Memory.GetUInt64Bit(ItemName);
	if (ExistedAddress > 0) { // key already exists
		pair<uint32_t, uint32_t> CurrentAddress = SplitAddres(ExistedAddress);

		uint32_t ReservedSpace = NormalizedItemAddress(CurrentAddress.second);

		if (ReservedSpace >= Item.size())
			ReplacementAddress = CurrentAddress.first; 	// replace if it is possible
		else
			DeleteItem(ItemName);						// delete to write at new location
	}

    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);
    if (Partition == NULL) return false;

    static uint32_t AddressToSave = 0;
    AddressToSave = (ReplacementAddress == 0xFFFFFFFF) ? GetFreeMemoryAddress() : ReplacementAddress;

    EraseRange(AddressToSave, NormalizedItemAddress(Item.size()));

    if (ESP_OK != esp_partition_write(Partition, AddressToSave, Item.c_str(), Item.size()))
    	return false;

    Memory.SetUInt64Bit(ItemName, JoinAddress(AddressToSave, Item.size()));

    AddressToSave += Item.size();

    SetFreeMemoryAddress(AddressToSave + Item.size());

	return true;
#endif
}

string DataEndpoint_t::GetItem(string ItemName) {
	NVS Memory(DataEndpoint_t::NVSArea);
#if (CONFIG_FIRMWARE_TARGET_SIZE_4MB)
	return Memory.GetString(ItemName);
#else
	if (ItemName == FREE_MEMORY_NVS)
		return "";

    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);

    if (Partition == NULL) return "";

    pair<uint32_t, uint32_t> AddressAndSize = SplitAddres(Memory.GetUInt64Bit(ItemName));

    char ReadBuffer[AddressAndSize.second];
    esp_err_t Result = esp_partition_read(Partition, AddressAndSize.first, ReadBuffer, AddressAndSize.second);

    if (Result != ESP_OK) return "";

	return string(ReadBuffer, AddressAndSize.second);
#endif
}

bool DataEndpoint_t::DeleteItem(string ItemName) {
	NVS Memory(DataEndpoint_t::NVSArea);
#if (CONFIG_FIRMWARE_TARGET_SIZE_4MB)
	return Memory.DeleteItem(ItemName);
#else
	if (ItemName == FREE_MEMORY_NVS)
		return false;

    pair<uint32_t, uint32_t> AddressAndSize = SplitAddres(Memory.GetUInt64Bit(ItemName));

    if (AddressAndSize.second > 0)
    	EraseRange(AddressAndSize.first, NormalizedItemAddress(AddressAndSize.second));

    Memory.Erase(ItemName);
    Memory.Commit();

    Defragment();

    return true;
#endif
}

bool DataEndpoint_t::DeleteStartedWith(string Key) {
	NVS Memory(DataEndpoint_t::NVSArea);
#if (CONFIG_FIRMWARE_TARGET_SIZE_4MB)
	return Memory.EraseStartedWith(ItemName);
#else
	if (Key == FREE_MEMORY_NVS)
		return false;

    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);
    if (Partition == NULL) return false;

    for (string Key : Memory.FindAllStartedWith(Key))
    {
        pair<uint32_t, uint32_t> AddressAndSize = SplitAddres(Memory.GetUInt64Bit(Key));
        ::esp_partition_erase_range(Partition, AddressAndSize.first, AddressAndSize.second);
        Memory.Erase(Key);
    }

    Memory.Commit();

    Defragment();

    return true;
#endif
}

void DataEndpoint_t::EraseAll() {
	NVS Memory(DataEndpoint_t::NVSArea);
	Memory.EraseNamespace();

	const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);

    if (Partition != NULL)
    	::esp_partition_erase_range(Partition, 0, Partition->size);
}

void DataEndpoint_t::Debug(string Tag) {
    const esp_partition_t *Partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PartitionName);

    if (Partition == NULL) return;

	nvs_handle handle;
	nvs_open(DataEndpoint_t::NVSArea.c_str(), NVS_READWRITE, &handle);

	if (Tag!="") Tag = ", Tag: " + Tag;
	ESP_LOGE("Data debug", "Output%s", Tag.c_str());

	nvs_iterator_t it = ::nvs_entry_find(NVS_DEFAULT_PART_NAME, DataEndpoint_t::NVSArea.c_str(), NVS_TYPE_ANY);
	while (it != NULL)
	{
		nvs_entry_info_t info;
		nvs_entry_info(it, &info);
		it = nvs_entry_next(it);

		uint64_t AddressAndSize;
		esp_err_t nvs_err = nvs_get_u64(handle, info.key, &AddressAndSize);

		if (nvs_err == ESP_OK)
		{
			uint32_t Address 	= (uint32_t)(AddressAndSize >> 32);
			uint32_t Size		= (uint32_t)((AddressAndSize << 32) >> 32);

		    char ReadBuffer[Size];
		    esp_err_t Result = esp_partition_read(Partition, Address, ReadBuffer, Size);

			ESP_LOGE("Data debug", "Item: %s Address %d Size %d", info.key, Address, Size);
			ESP_LOGE("Data debug", "Value: %s", string(ReadBuffer, Size).c_str());
		}
	}

	ESP_LOGE("!", "-------------");

	nvs_close(handle);
}

