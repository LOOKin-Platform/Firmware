/*
*    ISR.h
*    Class to control interrupts
*
*/
#ifndef DRIVERS_ISR_H_
#define DRIVERS_ISR_H_

using namespace std;

#include <cmath>
#include <map>

#include "driver/timer.h"
#include "esp_log.h"

typedef void (*TimerCallback)(void *);

/**
 * @brief Interface to ISR functions.
 */
class ISR {
	public:
		ISR();

		class HardwareTimer {
			public:
				HardwareTimer(timer_group_t, timer_idx_t, uint64_t = 100, TimerCallback = NULL);
				~HardwareTimer();

				void SetCallback(TimerCallback);
				void Start();
				void Stop();

			private:
				intr_handle_t 	s_timer_handle;
				timer_group_t 	TimerGroup;
				timer_idx_t		TimerIndex;
		};
};

#endif /* DRIVERS_ISR_H_ */
