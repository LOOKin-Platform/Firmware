/*
*    PartitionsAPI.h
*    Direct access ESP32 memory
*
*/

#ifndef DRIVERS_MEMORY_PARTITIONS_H_
#define DRIVERS_MEMORY_PARTITIONS_H_

using namespace std;

#include "esp_partition.h"
#include <esp_err.h>
/**
 * @brief Interface to direct functions interacting with memory.
 */

class PartitionAPI {
	public:

		static esp_err_t 	ErasePartition(char *);
};

#endif
