/*
*    Time.cpp
*    Class designed to work with time
*
*/

#ifndef LIBS_TIME_H_
#define LIBS_TIME_H_

using namespace std;

#include <string>

#include "Memory.h"
#include "FreeRTOSWrapper.h"
#include "HTTPClient.h"

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
    static float      TimezoneOffset;

    static uint32_t		Uptime();
    static uint64_t   UptimeU();
    static uint32_t   Unixtime();
    static DateTime_t DateTime();
    static string     UnixtimeString();
    static void       SetTime(string CurrentTime);

    static void       SetTimezone();
    static void       SetTimezone(string TimezoneOffset);
    static float      Timezone();
    static string     TimezoneStr();
    static bool       IsUptime(uint32_t Time);

    static uint32_t   UptimeToUnixTime(uint32_t);

    static void       ServerSync(const char* URL);

    // HTTP Callbacks
    /*
    static void ReadStarted (const char *IP);
    static bool ReadBody    (char Data[], int DataLen, const char *IP);
    static void ReadFinished(const char *IP);
     */
    static void Aborted     (const char *IP);

  private:
    static string     ReadBuffer;
};

#endif
