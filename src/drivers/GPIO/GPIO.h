/*
 * GPIO.h
 */

#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_
#include <driver/gpio.h>
/**
 * @brief Interface to GPIO functions.
 */
class GPIO {
public:
	GPIO();
	static bool IsInRange(gpio_num_t pin);
	static bool Read(gpio_num_t pin);
	static void Write(gpio_num_t pin, bool value);

	// Functions for PINs setting up
	static void Setup(gpio_num_t);
};

#endif /* DRIVERS_GPIO_H_ */
