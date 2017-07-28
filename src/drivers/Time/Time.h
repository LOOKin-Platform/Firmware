#ifndef DRIVERS_TIME_H_
#define DRIVERS_TIME_H_

/*
  Классы для работы со временем на уровне устройства
*/

#include <nvs.h>
#include <string>

#include "HTTPClient/HTTPClient.h"

using namespace std;

#define  NVSTimeTimezone  "Timezone"

class Time {
  public:
    static uint32_t Offset;
    static int8_t   TimezoneOffset;

    static uint32_t Uptime();
    static uint32_t Unixtime();
    static string   UnixtimeString();
    static void     SetTime(string CurrentTime);


    static void     SetTimezone();
    static void     SetTimezone(string TimezoneOffset);
    static int8_t   Timezone();
    static string   TimezoneStr();

    static void     ServerSync(string Host, string Path);
};

class TimeHTTPClient_t: public HTTPClient_t {
  public:
    void ReadStarted() override;
    bool ReadBody(char Data[], int DataLen) override;
    bool ReadFinished() override;
  private:
    string ReadBuffer;
};

#endif
