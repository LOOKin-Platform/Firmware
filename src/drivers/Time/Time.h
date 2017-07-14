#ifndef DRIVERS_TIME_H_
#define DRIVERS_TIME_H_
#include <nvs.h>
#include <string>

enum TimeFormat { SECONDS };

class Time {
  public:
  	Time();
  	virtual ~Time();

    static long int GetTime();
    static string   GetTimeString();
};

#endif
