/*
*    OTA class
*
*/
#ifndef DRIVERS_OTA_H_
#define DRIVERS_OTA_H_

#include <string.h>

#include "esp_ota_ops.h"

#include "HTTPClient/HTTPClient.h"
#include "Globals.h"

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
    static bool ReadFinished(char []);
    static void Aborted(char []);

  private:
    static int               BinaryFileLength;
    static esp_ota_handle_t  OutHandle;
    static esp_partition_t   OperatePartition;
};

#endif
