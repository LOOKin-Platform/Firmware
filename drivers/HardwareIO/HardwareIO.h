/*
 *    GPIO.h
 *    Class to rule pin outputs and inputs
 *
 */
#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_

using namespace std;

#include <cmath>
#include <map>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include <driver/rtc_io.h>

#include "DateTime.h"
#include "Settings.h"

#include "esp_log.h"

#define PWM_FADING_LENGTH 2000

/**
 * @brief Interface to GPIO functions.
 */
class GPIO {
	public:
		struct PWMCacheItem {
			uint8_t 	Value 	= 0;
			uint32_t 	Updated = 0;
			PWMCacheItem(uint8_t dValue = 0, uint32_t dUpdated = 0) : Value(dValue), Updated(dUpdated) {}
		};

		GPIO();

		// Regular work with PIN outputs and inputs
		static void 	Setup(gpio_num_t);
		static bool 	IsInRange(gpio_num_t pin);
		static bool 	Read(gpio_num_t pin);
		static void 	Write(gpio_num_t pin, bool value);

		// PWM to rule diods
		static void 	SetupPWM(gpio_num_t GPIO, ledc_timer_t TimerIndex, ledc_channel_t PWMChannel);
		static uint8_t 	PWMValue(ledc_channel_t PWMChannel);
		static void 	PWMFadeTo(ledc_channel_t PWMChannel, uint8_t Duty = 255, uint16_t FadeTime = PWM_FADING_LENGTH);
		static void 	PWMFadeTo(Settings_t::GPIOData_t::Color_t::Item_t, uint8_t Duty = 255, uint16_t FadeTime = PWM_FADING_LENGTH);

		static map<ledc_channel_t, PWMCacheItem> PWMValuesCache;
};

#endif /* DRIVERS_GPIO_H_ */
