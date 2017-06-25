#ifndef DRIVERS_TIME_H_
#define DRIVERS_TIME_H_
#include <nvs.h>
#include <string>

class Time {
  public:
  	Time();
  	virtual ~Time();

    static string GetUnixTime();
};

#endif
