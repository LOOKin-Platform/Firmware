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
    //void ChangeBootPartition();
};

class OTAHTTPClient_t: public HTTPClient_t {
  public:
    void ReadStarted() override;
    bool ReadBody(char Data[], int DataLen) override;
    bool ReadFinished() override;

    void Aborted() override;
  private:
    int               BinaryFileLength;
    esp_ota_handle_t  OutHandle;
    esp_partition_t   OperatePartition;
};

#endif
