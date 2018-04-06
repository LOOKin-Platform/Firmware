/*
*    Time.cpp
*    Class designed to work with time
*
*/

#ifndef DRIVERS_TIME_H_
#define DRIVERS_TIME_H_

using namespace std;

#include <string>

#include "Memory.h"
#include "FreeRTOSWrapper.h"
#include "HTTPClient.h"

typedef void (*TimeSynced)();

#define  NVSTimeTimezone  "Timezone"

struct DateTime_t {
  uint8_t   Hours     = 0;
  uint8_t   Minutes   = 0;
  uint8_t   Seconds   = 0;

  uint8_t   Day       = 0;
  uint8_t   Month     = 0;
  uint16_t  Year      = 0;

  uint8_t   DayOfWeek = 0;
};

class Time {
  public:
    static uint32_t   Offset;
    static int8_t     TimezoneOffset;

    static uint32_t   Uptime();
    static uint32_t   UptimeU();
    static uint32_t   Unixtime();
    static DateTime_t DateTime();
    static string     UnixtimeString();
    static void       SetTime(string CurrentTime);

    static void       SetTimezone();
    static void       SetTimezone(string TimezoneOffset);
    static int8_t     Timezone();
    static string     TimezoneStr();
    static bool       IsUptime(uint32_t Time);

    static void       ServerSync(string Host, string Path);

    // HTTP Callbacks
    static void ReadStarted (char IP[]);
    static bool ReadBody    (char Data[], int DataLen, char IP[]);
    static void ReadFinished(char IP[]);
    static void Aborted     (char IP[]);

  private:
    static string     ReadBuffer;
};

#endif
