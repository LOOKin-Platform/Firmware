/*
*    ISR.cpp
*    Class to control interrupts
*
*/

#include "ISR.h"

/**
 * @brief Class instance constructor.
 */
ISR::ISR() {}

/**
 * @brief Init hardware timer.
 *
 * @param [in] TimerGroup ESP32 Timer group.
 * @param [in] TimerIndex ESP32 Timer Index.
 * @param [in] Interval	 ESP32 Timer Interval.
 */
ISR::HardwareTimer::HardwareTimer(timer_group_t TimerGroup, timer_idx_t TimerIndex, uint64_t Interval, TimerCallback Callback, void *Param) {
	this->TimerGroup = TimerGroup;
	this->TimerIndex = TimerIndex;

	if (TimerGroup == TIMER_GROUP_MAX || TimerIndex == TIMER_MAX)
		return;

	timer_config_t config = {
            .alarm_en = true,
            .counter_en = false,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = true,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TimerGroup, TimerIndex, &config);
    timer_pause(TimerGroup, TimerIndex);
    timer_set_counter_value(TimerGroup, TimerIndex, 0x00000000ULL);
    timer_set_alarm_value(TimerGroup, TimerIndex, Interval);
    timer_enable_intr(TimerGroup, TimerIndex);
    timer_isr_register(TimerGroup, TimerIndex, Callback, Param, 0, &s_timer_handle);
}

ISR::HardwareTimer::~HardwareTimer() {
}

void ISR::HardwareTimer::Start() {
	if (TimerGroup != TIMER_GROUP_MAX && TimerIndex != TIMER_MAX)
		timer_start(TimerGroup, TimerIndex);
}

void ISR::HardwareTimer::Pause() {
	if (TimerGroup != TIMER_GROUP_MAX && TimerIndex != TIMER_MAX)
		timer_pause(TimerGroup, TimerIndex);
}

void ISR::HardwareTimer::Stop() {
	if (TimerGroup != TIMER_GROUP_MAX && TimerIndex != TIMER_MAX) {
		timer_pause(TimerGroup, TimerIndex);
		timer_disable_intr(TimerGroup, TimerIndex);
	}
}

void ISR::HardwareTimer::CallbackPrefix(timer_group_t TimerGroup, timer_idx_t Timer) {
	if (TimerGroup == TIMER_GROUP_0) {
		if (Timer == TIMER_0) {
			TIMERG0.int_clr_timers.t0 = 1;
			TIMERG0.hw_timer[0].config.alarm_en = 1;
		}

		if (Timer == TIMER_1) {
			TIMERG0.int_clr_timers.t1 = 1;
			TIMERG0.hw_timer[1].config.alarm_en = 1;
		}
	}

	if (TimerGroup == TIMER_GROUP_1) {
		if (Timer == TIMER_0) {
			TIMERG1.int_clr_timers.t0 = 1;
			TIMERG1.hw_timer[0].config.alarm_en = 1;
		}

		if (Timer == TIMER_1) {
			TIMERG1.int_clr_timers.t1 = 1;
			TIMERG1.hw_timer[1].config.alarm_en = 1;
		}
	}
}
