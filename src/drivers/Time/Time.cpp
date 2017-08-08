/*
*    Time.cpp
*    Class designed to work with time
*
*/

#include "Time.h"

#include "JSON/JSON.h"
#include <string>
#include <sys/time.h>
#include <cmath>
#include <limits>

#include <NVS/NVS.h>
#include <Converter.h>

static char tag[] = "Time";
static string NVSTimeArea = "Time";

uint32_t  Time::Offset = 0;
int8_t    Time::TimezoneOffset = 0;
string    Time::ReadBuffer = "";

uint32_t Time::Uptime() {
  struct timeval Now;
  ::gettimeofday(&Now, NULL);

  return Now.tv_sec;
}

uint32_t Time::Unixtime() {
  return Uptime() + Offset + TimezoneOffset*3600;
}

DateTime_t Time::DateTime() {
  uint32_t TimeToParse = Unixtime();

  DateTime_t DateTime;

  DateTime.Seconds  = TimeToParse % 60;
  DateTime.Minutes  = (TimeToParse / 60) % 60;
  DateTime.Hours    = (int)(TimeToParse / 3600) % 24;

  int      AllDays  = (int)(TimeToParse / 86400);

  DateTime.Year     = floor(AllDays / 365.25) + 1970;
  DateTime.DayOfWeek= (AllDays + 4) % 7;

  int Z = AllDays + 719468;
  const int Era = (Z >= 0 ? Z : Z - 146096) / 146097;
  const unsigned Doe = static_cast<unsigned>(Z - Era * 146097);           // [0, 146096]
  const unsigned Yoe = (Doe - Doe/1460 + Doe/36524 - Doe/146096) / 365;   // [0, 399]
  const unsigned Doy = Doe - (365*Yoe +Yoe/4 - Yoe/100);                  // [0, 365]
  const unsigned Mp = (5*Doy + 2)/153;                                    // [0, 11]

  DateTime.Day   = Doy - (153*Mp+2)/5 + 1;
  DateTime.Month = Mp + (Mp < 10 ? 3 : -9);

  return DateTime;
}

string Time::UnixtimeString() {
  return Converter::ToString(Time::Unixtime());
}

void Time::SetTime(string CurrentTime) {
  Time::Offset = (uint32_t) atol (CurrentTime.c_str()) - Time::Uptime();
}

void Time::SetTimezone() {
  NVS *Memory = new NVS(NVSTimeArea);
  uint8_t TimeZoneTemp = Memory->GetInt8Bit(NVSTimeTimezone);
  if (TimeZoneTemp!=0) Time::TimezoneOffset = TimeZoneTemp - 25;
}

void Time::SetTimezone(string TimezoneOffset) {
  Time::TimezoneOffset = (uint32_t) atoi (TimezoneOffset.c_str());

  NVS *Memory = new NVS(NVSTimeArea);
  Memory->SetInt8Bit(NVSTimeTimezone, Time::TimezoneOffset + 25);
  Memory->Commit();
}

int8_t Time::Timezone() {
  return Time::TimezoneOffset;
}

string Time::TimezoneStr() {
  string Result = Converter::ToString(Time::Timezone());
  return (Time::Timezone() > 0) ? "+" + Result : Result;
}

void Time::ServerSync(string Host, string Path) {
  if (Offset != 0) return;

  ESP_LOGI(tag, "Time sync started");
  HTTPClient::Query(Host, 80, Path, QueryType::GET, "", true, &ReadStarted, &ReadBody, &ReadFinished, &Aborted);
}

void Time::ReadStarted(char IP[]) {
  ReadBuffer = "";
}

bool Time::ReadBody(char Data[], int DataLen, char IP[]) {
  ReadBuffer += Data;
  return true;
};

bool Time::ReadFinished(char IP[]) {
  if (ReadBuffer.length() == 0) return false;

  JSON JSONObject(ReadBuffer);

  string GMTTime = JSONObject.GetItem("GMT");
  if (GMTTime.empty()) {
    ESP_LOGE(tag, "No time info found");
    return false;
  }

  Time::SetTime(GMTTime);
  ESP_LOGI(tag, "Time received: %s", Time::UnixtimeString().c_str());

  return true;
};

void Time::Aborted(char IP[]) {
  ESP_LOGE(tag, "Failed to retrieve time from server");
}
