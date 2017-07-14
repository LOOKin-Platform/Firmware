using namespace std;

#include "Time.h"
#include <sys/time.h>
#include <sstream>

Time::Time() {}

long int Time::GetTime() {
  struct timeval Now;
  ::gettimeofday(&Now, NULL);
  return Now.tv_sec;
}

string Time::GetTimeString() {
  stringstream TimeString;
  TimeString << Time::GetTime();
  return TimeString.str();
}
