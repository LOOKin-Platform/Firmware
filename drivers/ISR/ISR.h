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
#include <vector>

#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_log.h"

typedef void (*ISRCallback)(void *);
typedef void (*ISRTimerCallback)(void *);

class ISR {
	public:
		ISR();

		struct InterruptData_t {
			gpio_int_type_t	Type;
			ISRCallback		Callback;
		};

		static void AddInterrupt(gpio_num_t GPIO = GPIO_NUM_0, gpio_int_type_t Type = GPIO_INTR_DISABLE, ISRCallback Callback = NULL);
		static void RemoveInterrupt(gpio_num_t);
		static void Install();

		class HardwareTimer {
			public:
				HardwareTimer(timer_group_t = TIMER_GROUP_MAX, timer_idx_t = TIMER_MAX, uint64_t = 100, ISRTimerCallback = NULL, void *Param = NULL);
				~HardwareTimer();

				void SetCallback(ISRTimerCallback);
				void Start();
				void Pause();
				void Stop();

				static void CallbackPrefix(timer_group_t, timer_idx_t);

			private:
				intr_handle_t 	s_timer_handle;
				timer_group_t 	TimerGroup;
				timer_idx_t		TimerIndex;
		};

	private:
		static map<gpio_num_t, InterruptData_t> Interrupts;
};

#endif /* DRIVERS_ISR_H_ */
