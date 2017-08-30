/*
*    GPIO.cpp
*    Class to rule pin outputs and inputs
*
*/

#include "HardwareIO.h"
#include <driver/gpio.h>

/**
 * @brief Class instance constructor.
 */
GPIO::GPIO() {
	// TODO Auto-generated constructor stub

}

/**
 * @brief Determine if the pin is a valid pin for an ESP32 (i.e. is it in range).
 *
 * @param [in] pin The pin number to validate.
 * @return The value of true if the pin is valid and false otherwise.
 */
bool GPIO::IsInRange(gpio_num_t pin) {
	if (pin>=0 && pin<=39) {
		return true;
	}
	return false;
}

/**
 * @brief Read a value from the given pin.
 * @param [in] pin The pin to read from.
 */
bool GPIO::Read(gpio_num_t pin) {
	return ::gpio_get_level(pin);
}

/**
 * @brief Write a value to the given pin.
 *
 * @param [in] pin The gpio pin to change.
 * @param [out] value The value to be written to the pin.
 */
void GPIO::Write(gpio_num_t pin, bool value) {
	::gpio_set_level(pin, value);
}

/**
 * @brief Setting up pin for Output.
 *
 * @param [in] pin to setup.
 */

void GPIO::Setup(gpio_num_t PIN_NUM) {
	::gpio_pad_select_gpio(PIN_NUM);
	::gpio_set_direction(PIN_NUM, GPIO_MODE_INPUT_OUTPUT);
}
