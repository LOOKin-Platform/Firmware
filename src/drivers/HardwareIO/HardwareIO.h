/*
*    GPIO.h
*    Class to rule pin outputs and inputs
*
*/

#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "cmath"

/**
 * @brief Interface to GPIO functions.
 */
class GPIO {
public:
	GPIO();
	static bool 		IsInRange(gpio_num_t pin);
	static bool 		Read(gpio_num_t pin);
	static void 		Write(gpio_num_t pin, bool value);

	static uint8_t 	PWMValue (ledc_channel_t PWMChannel);
	static void 		PWMFadeTo(ledc_channel_t PWMChannel, uint8_t Duty = 255);

	// Functions for PINs setting up
	static void Setup(gpio_num_t);
	static void SetupPWM(gpio_num_t GPIO, ledc_timer_t TimerIndex, ledc_channel_t PWMChannel);
};

#endif /* DRIVERS_GPIO_H_ */
