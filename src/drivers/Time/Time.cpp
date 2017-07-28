/*
  Классы для работы со временем на уровне устройства
*/

#include "Time.h"

#include "JSON.h"
#include <string>
#include <sys/time.h>

#include <NVS/NVS.h>
#include <Tools.h>

static char tag[] = "Time";
static string NVSTimeArea = "Time";

uint32_t  Time::Offset = 0;
int8_t    Time::TimezoneOffset = 0;

uint32_t Time::Uptime() {
  struct timeval Now;
  ::gettimeofday(&Now, NULL);

  return Now.tv_sec;
}

uint32_t Time::Unixtime() {
  return Uptime() + Offset + TimezoneOffset*3600;
}

string Time::UnixtimeString() {
  return Tools::ToString(Time::Unixtime());
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
  string Result = Tools::ToString(Time::Timezone());
  return (Time::Timezone() > 0) ? "+" + Result : Result;
}

void Time::ServerSync(string Host, string Path) {
  if (Offset != 0) return;

  ESP_LOGI(tag, "Time sync started");

  TimeHTTPClient_t* HTTPClient = new TimeHTTPClient_t();
  HTTPClient->Hostname        = Host;
  HTTPClient->ContentURI      = Path;

  HTTPClient->Request();
}

void TimeHTTPClient_t::ReadStarted() {
  ReadBuffer = "";
}

bool TimeHTTPClient_t::ReadBody(char Data[], int DataLen) {
  ReadBuffer += Data;
  return true;
};

bool TimeHTTPClient_t::ReadFinished() {
  if (ReadBuffer.length() == 0) return false;

  map<string,string> ServerData = JSON_t::MapFromJsonString(ReadBuffer);

  if (ServerData.empty()) {
    ESP_LOGE(tag, "Incorrect Time info received");
    return false;
  }

  if (ServerData.count("GMT") == 0) {
    ESP_LOGE(tag, "No time info found");
    return false;
  }
  else {
    Time::SetTime(ServerData["GMT"]);
    ESP_LOGI(tag, "Time received: %s", Time::UnixtimeString().c_str());
    return true;
  }

  return true;
};
