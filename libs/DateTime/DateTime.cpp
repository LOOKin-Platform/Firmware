/*
*    Time.cpp
*    Class designed to work with time
*
*/

#define _USE_32BIT_TIME_T

#include "DateTime.h"

#include <string>
#include <sys/time.h>
#include <cmath>
#include <limits>

#include "Log.h"
#include "JSON.h"
#include "Converter.h"
#include "Memory.h"

#include <time.h>
#include "esp_sntp.h"



const char tag[] 			= "Time";
const char NVSTimeArea[] 	= "Time";

uint32_t    Time::Offset = 0;
float		Time::TimezoneOffset = 0;
string      Time::ReadBuffer = "";


uint32_t Time::Uptime() {
	struct timeval Now;
	::gettimeofday(&Now, NULL);

	return Now.tv_sec;
}

uint64_t Time::UptimeU() {
	struct timeval Now;
	::gettimeofday(&Now, NULL);

	return Now.tv_sec * 1000000 + Now.tv_usec;
}

uint32_t Time::Unixtime() {
    time_t now;
    time(&now);
    return now + Offset + TimezoneOffset*3600;
}

DateTime_t Time::DateTime() {
	DateTime_t DateTime;
    time_t now;
    struct tm timeInfo;
    now = Time::Unixtime();
    localtime_r(&now, &timeInfo);

    DateTime.Year = timeInfo.tm_year + 1900;
    DateTime.Month = timeInfo.tm_mon;
    DateTime.Day = timeInfo.tm_mday;
    DateTime.DayOfWeek = timeInfo.tm_wday;
    DateTime.Hours = timeInfo.tm_hour;
    DateTime.Minutes = timeInfo.tm_min;
    DateTime.Seconds = timeInfo.tm_sec;
	return DateTime;
}

string Time::UnixtimeString() {
	return Converter::ToString(Time::Unixtime());
}

void Time::SetTime(string CurrentTime) {
	Time::Offset = (uint32_t) atol (CurrentTime.c_str()) - Time::Uptime();
}

void Time::SetTimezone() {
	NVS Memory(NVSTimeArea);

	string TimeZoneString = Memory.GetString(NVSTimeTimezone);

	Time::TimezoneOffset = Converter::ToFloat(TimeZoneString);
}

void Time::SetTimezone(string TimezoneOffset) {
	Time::TimezoneOffset = Converter::ToFloat(TimezoneOffset);

	NVS Memory(NVSTimeArea);

	Memory.SetString(NVSTimeTimezone, TimezoneOffset);
	Memory.Commit();
}

float Time::Timezone() {
	return Time::TimezoneOffset;
}

string Time::TimezoneStr() {
	string Result = Converter::ToString<float>(Time::Timezone());
	return (Time::Timezone() > 0) ? "+" + Result : Result;
}

bool Time::IsUptime(uint32_t Time) {
	return (Time < 31536000) ? true : false;
}

uint32_t Time::UptimeToUnixTime(uint32_t Uptime) {
	return Uptime + Offset + (uint32_t)(TimezoneOffset*3600);
}


void Time::ServerSync(string URL) {
	if (Offset != 0)
		return;

	ESP_LOGI(tag, "Time sync started");

    std::string* url = new std::string(URL);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, url->c_str());
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_init();
}


void Time::Aborted(const char *IP) {
	ESP_LOGE(tag, "Failed to retrieve time from server");
}
