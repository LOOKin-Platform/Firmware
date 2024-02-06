/*
*    ISR.cpp
*    Class to control interrupts
*
*/

#include "ISR.h"
#include <string.h>


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

    gpio_config_t io_conf = {};

    for (std::pair<gpio_num_t, InterruptData_t> Item : ISR::Interrupts) {
	    io_conf.intr_type    	= Item.second.Type;
		io_conf.mode			= GPIO_MODE_INPUT;
	    io_conf.pin_bit_mask 	= (1ULL << Item.first);
	    io_conf.pull_down_en 	= GPIO_PULLDOWN_ENABLE;
	    io_conf.pull_up_en   	= GPIO_PULLUP_ENABLE;

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
ISR::HardwareTimer::HardwareTimer(uint64_t Interval, gptimer_alarm_cb_t Callback, void *Param) {
	this->TimerHandle = NULL;

	gptimer_config_t timer_config;
	memset(&timer_config, 0, sizeof(timer_config));

	timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
	timer_config.direction = GPTIMER_COUNT_UP;
	timer_config.resolution_hz = 1 * 1000 * 1000; // 1MHz, 1 tick = 1us
	timer_config.flags.intr_shared  = 1;

	gptimer_new_timer(&timer_config, &TimerHandle);
	gptimer_stop(TimerHandle);
	gptimer_set_raw_count(TimerHandle, 0x00000000ULL);

	gptimer_alarm_config_t alarm_config;
	memset(&alarm_config, 0, sizeof(alarm_config));
	alarm_config.alarm_count = Interval;
	alarm_config.flags.auto_reload_on_alarm = true;
		
	gptimer_set_alarm_action(TimerHandle, &alarm_config);

	gptimer_event_callbacks_t CallbacksStruct;
	memset(&CallbacksStruct, 0, sizeof(CallbacksStruct));
	CallbacksStruct.on_alarm = Callback;

	gptimer_register_event_callbacks(TimerHandle, &CallbacksStruct, Param);
}

ISR::HardwareTimer::~HardwareTimer() {
}

void ISR::HardwareTimer::Start() {
	if (TimerHandle != NULL)
		gptimer_start(TimerHandle);
}

void ISR::HardwareTimer::Pause() {
	if (TimerHandle != NULL)
		gptimer_stop(TimerHandle);
}

void ISR::HardwareTimer::Stop() {
	if (TimerHandle != NULL) {
		gptimer_stop(TimerHandle);
		gptimer_disable(TimerHandle);
	}
}
