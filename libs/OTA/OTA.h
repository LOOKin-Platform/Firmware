/*
*    OTA.h
*    Over the Air updates implementation
*
*/

#ifndef LIBS_OTA_H_
#define LIBS_OTA_H_

#include <string.h>

#include <esp_spi_flash.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#include <esp_err.h>
#include <esp_system.h>
#include <esp_log.h>

#include "HTTPClient.h"
#include "FreeRTOSWrapper.h"

using namespace std;

#define BUFFSIZE      1024
#define TEXT_BUFFSIZE 1024

typedef void (*OTAStarted)			();
typedef void (*OTAFileDoesntExist)	();

class OTA {
	public:
		OTA();
		static void 				Update(string URL, OTAStarted Started = NULL, OTAFileDoesntExist FileDoesntExist = NULL);
		static esp_err_t			PerformUpdate(string URL);

		static bool 				Rollback();

		// HTTP OTA Callbacks
		static void 				ReadStarted		(char [] = '\0');
		static bool 				ReadBody		(char Data[], int DataLen, char[] = '\0');
		static void 				ReadFinished	(char [] = '\0');
		static void 				Aborted			(char [] = '\0');

		static uint8_t				Attempts;
		static bool					IsOTAFileExist;
		static bool					IsFileCheckEnded;

	private:
		static int					BinaryFileLength;
		static esp_ota_handle_t		OutHandle;
		static esp_partition_t		OperatePartition;

		static FreeRTOS::Timer*		DelayedRebootTimer;
		static void					DelayedRebootTask(FreeRTOS::Timer *);

};

#endif
