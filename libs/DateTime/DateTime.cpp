/*
*    Time.cpp
*    Class designed to work with time
*
*/

#include "DateTime.h"

#include <string>
#include <sys/time.h>
#include <cmath>
#include <limits>

#include "Log.h"
#include "JSON.h"
#include "Converter.h"
#include "Memory.h"

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
	const int Era 		= (Z >= 0 ? Z : Z - 146096) / 146097;
	const unsigned Doe 	= static_cast<unsigned>(Z - Era * 146097);           // [0, 146096]
	const unsigned Yoe 	= (Doe - Doe/1460 + Doe/36524 - Doe/146096) / 365;   // [0, 399]
	const unsigned Doy 	= Doe - (365*Yoe +Yoe/4 - Yoe/100);                  // [0, 365]
	const unsigned Mp 	= (5*Doy + 2)/153;                                   // [0, 11]

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
	HTTPClient::Query(URL, QueryType::GET, true, false, &ReadStarted, &ReadBody, &ReadFinished, &Aborted);
}

void Time::ReadStarted(const char *IP) {
	ReadBuffer = "";
}

bool Time::ReadBody(char Data[], int DataLen, const char *IP) {
	ReadBuffer += Data;
	return true;
};

void Time::ReadFinished(const char *IP) {
	if (ReadBuffer.length() == 0) {
		ESP_LOGE(tag, "Empty data received");
		return;
	}

	JSON JSONObject(ReadBuffer);

	string GMTTime = JSONObject.GetItem("GMT");
	if (GMTTime.empty()) {
		ESP_LOGE(tag, "No time info found");
		return;
	}

	Time::SetTime(GMTTime);
	ESP_LOGI(tag, "Time received: %s", Time::UnixtimeString().c_str());

	Log::CorrectTime();
}

void Time::Aborted(const char *IP) {
	ESP_LOGE(tag, "Failed to retrieve time from server");
}
