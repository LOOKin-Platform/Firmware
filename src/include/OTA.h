/*
*    OTA class
*
*/

#ifndef DRIVERS_OTA_H_
#define DRIVERS_OTA_H_

#include <string.h>

#include <esp_spi_flash.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#include <esp_err.h>
#include <esp_system.h>
#include <esp_log.h>

#include "HTTPClient.h"

using namespace std;

#define BUFFSIZE      1024
#define TEXT_BUFFSIZE 1024

class OTA_t {
  public:
    OTA_t();
    void Update(string URL);

    // HTTP Callbacks
    static void ReadStarted(char []);
    static bool ReadBody(char Data[], int DataLen, char[]);
    static void ReadFinished(char []);
    static void Aborted(char []);

  private:
    static int               BinaryFileLength;
    static esp_ota_handle_t  OutHandle;
    static esp_partition_t   OperatePartition;
};

#endif
