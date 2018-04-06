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
ISR::HardwareTimer::HardwareTimer(timer_group_t TimerGroup, timer_idx_t TimerIndex, uint64_t Interval, TimerCallback Callback) {
	this->TimerGroup = TimerGroup;
	this->TimerIndex = TimerIndex;

	timer_config_t config = {
            .alarm_en = true,
            .counter_en = false,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = true,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TimerGroup, TimerIndex, &config);
    timer_set_counter_value(TimerGroup, TimerIndex, 0);
    timer_set_alarm_value(TimerGroup, TimerIndex, Interval);
    timer_enable_intr(TimerGroup, TimerIndex);
    timer_isr_register(TimerGroup, TimerIndex, Callback, NULL, 0, &s_timer_handle);
}

ISR::HardwareTimer::~HardwareTimer() {
}

void ISR::HardwareTimer::Start() {
    timer_start(TimerGroup, TimerIndex);
}
