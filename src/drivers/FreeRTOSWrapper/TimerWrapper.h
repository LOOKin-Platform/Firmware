#ifndef DRIVERS_FREERTOSTIMER_H_
#define DRIVERS_FREERTOSTIMER_H_
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <freertos/timers.h>

#include <string>

using namespace std;

/**
 * @brief Wrapper around the %FreeRTOS timer functions.
 */
class Timer_t {
public:
	Timer_t(string Name, TickType_t period, UBaseType_t reload, void *data, void (*callback)(Timer_t *pTimer));
	virtual ~Timer_t();
	void ChangePeriod(TickType_t newPeriod, TickType_t blockTime=portMAX_DELAY);
	void *GetData();
	const char *GetName();
	TickType_t GetPeriod();
	void Reset(TickType_t blockTime=portMAX_DELAY);
	void Start(TickType_t blockTime=portMAX_DELAY);
	void Stop(TickType_t blockTime=portMAX_DELAY);
private:
	TimerHandle_t timerHandle;
	TickType_t period;
	void (*callback)(Timer_t *pTimer);
	static void internalCallback(TimerHandle_t xTimer);
};

#endif /* DRIVERS_FREERTOSTIMER_H_ */
