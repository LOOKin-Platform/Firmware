/*
*    ISR.cpp
*    Class to control interrupts
*
*/

#include "ISR.h"

map<gpio_num_t, ISR::InterruptData_t> ISR::Interrupts = {};

/**
 * @brief Class instance constructor.
 */
ISR::ISR() {}

void ISR::AddInterrupt(gpio_num_t sGPIO, gpio_int_type_t sType, ISRCallback sCallback) {
	InterruptData_t Data;
	Data.Type 		= sType;
	Data.Callback 	= sCallback;

	if (Interrupts.count(sGPIO) == 0)
		Interrupts.insert(pair<gpio_num_t, InterruptData_t>(sGPIO, Data));
	else
		Interrupts[sGPIO] = Data;
}

void ISR::RemoveInterrupt(gpio_num_t GPIO) {
	Interrupts.erase(GPIO);
}

void ISR::Install() {
	::gpio_uninstall_isr_service();

    gpio_config_t  io_conf;

    for (std::pair<gpio_num_t, InterruptData_t> Item : ISR::Interrupts) {
	    io_conf.intr_type    = Item.second.Type;
	    io_conf.pin_bit_mask = (1ULL << Item.first);
	    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
	    gpio_config( &io_conf);
	}

	::gpio_install_isr_service(0);

    for (std::pair<gpio_num_t, InterruptData_t> Item : ISR::Interrupts)
	    ::gpio_isr_handler_add(Item.first, Item.second.Callback, (void*) (uint32_t) Item.first);
}

/**
 * @brief Init hardware timer.
 *
 * @param [in] TimerGroup ESP32 Timer group.
 * @param [in] TimerIndex ESP32 Timer Index.
 * @param [in] Interval	 ESP32 Timer Interval.
 */
ISR::HardwareTimer::HardwareTimer(timer_group_t TimerGroup, timer_idx_t TimerIndex, uint64_t Interval, ISRTimerCallback Callback, void *Param) {
	this->TimerGroup = TimerGroup;
	this->TimerIndex = TimerIndex;

	if (TimerGroup == TIMER_GROUP_MAX || TimerIndex == TIMER_MAX)
		return;

	timer_config_t config = {
            .alarm_en 		= TIMER_ALARM_EN,
            .counter_en 	= TIMER_PAUSE,
            .intr_type 		= TIMER_INTR_LEVEL,
            .counter_dir 	= TIMER_COUNT_UP,
            .auto_reload 	= TIMER_AUTORELOAD_EN,
            .divider 		= 80   /* 1 us per tick */
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
