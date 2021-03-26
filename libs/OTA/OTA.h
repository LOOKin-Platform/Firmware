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

#include "Settings.h"

using namespace std;

#define BUFFSIZE      1024
#define TEXT_BUFFSIZE 1024

typedef void (*OTAStarted)	();
typedef void (*OTAFailed)	();

class OTA {
	public:
		OTA();
		static void 				Update(string URL, OTAStarted StartedCallback, OTAFailed FailedCallback = NULL);
		static esp_err_t			PerformUpdate(string URL);

		static bool 				Rollback();

		// HTTP OTA Callbacks
		static void 				ReadStarted		(char []);
		static bool 				ReadBody		(char Data[], int DataLen, char[]);
		static void 				ReadFinished	(char []);
		static void 				Aborted			(char []);

		static uint8_t				Attempts;
		static bool					IsOTAFileExist;
		static bool					IsFileCheckEnded;

	private:
		static OTAFailed			OTAFailedCallback;

		static int					BinaryFileLength;
		static esp_ota_handle_t		OutHandle;
		static esp_partition_t		OperatePartition;

		static FreeRTOS::Timer*		DelayedRebootTimer;
		static void					DelayedRebootTask(FreeRTOS::Timer *);

};

#endif
