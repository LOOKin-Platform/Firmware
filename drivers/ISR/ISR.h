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

#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"

typedef void (*ISRCallback)(void *);

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
				HardwareTimer(uint64_t = 100, gptimer_alarm_cb_t Callback = NULL, void *Param = NULL);
				~HardwareTimer();

				void SetCallback(gptimer_alarm_cb_t Callback);
				void Start();
				void Pause();
				void Stop();

			private:
				gptimer_handle_t TimerHandle;
		};

	private:
		static map<gpio_num_t, InterruptData_t> Interrupts;
};

#endif /* DRIVERS_ISR_H_ */
