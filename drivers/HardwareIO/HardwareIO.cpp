/*
*    GPIO.cpp
*    Class to rule pin outputs and inputs
*
*/

#include "HardwareIO.h"

/**
 * @brief Class instance constructor.
 */
GPIO::GPIO() {}
static char tag[]= "GPIO";

/**
 * @brief Determine if the pin is a valid pin for an ESP32 (i.e. is it in range).
 *
 * @param [in] pin The pin number to validate.
 * @return The value of true if the pin is valid and false otherwise.
 */
bool GPIO::IsInRange(gpio_num_t pin) {
	if (pin >= 0 && pin <= 39) {
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
 * @brief Set hold value to the given pin.
 *
 * @param [in] pin The gpio pin to change.
 * @param [out] value The value to be written to the pin.
 */
void GPIO::Hold(gpio_num_t pin, bool value) {
	if (value)
		::gpio_hold_en(pin);
	else
		::gpio_hold_dis(pin);
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

void GPIO::Setup(gpio_num_t Pin, gpio_mode_t Mode) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = Mode;

    io_conf.pin_bit_mask = (1ULL << Pin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    gpio_config(&io_conf);
}


map<ledc_channel_t, GPIO::PWMCacheItem> GPIO::PWMValuesCache = {{}};
bool GPIO::PWMIsInited = false;
/**
 * @brief Setting up pin for PWM.
 *
 * @param [in] GPIO			pin to setup.
 * @param [in] TimerIndex	LEDC Timer Index [0..3]
 * @param [in] PWMChannel	LEDC Channel number [0..7]
 */

void GPIO::SetupPWM(gpio_num_t GPIO, ledc_timer_t TimerIndex, ledc_channel_t PWMChannel) {
	if (GPIO == GPIO_NUM_0)
		return;

	if (!PWMIsInited)
		PWMIsInited = true;

	//::rtc_gpio_isolate(GPIO);

	ledc_timer_config_t ledc_timer;

	ledc_timer.duty_resolution	= LEDC_TIMER_8_BIT;
	ledc_timer.freq_hz			= 1000;
	ledc_timer.speed_mode		= LEDC_HIGH_SPEED_MODE;
	ledc_timer.timer_num		= TimerIndex;
	ledc_timer.clk_cfg			= LEDC_USE_REF_TICK;

	::ledc_timer_config(&ledc_timer);

	ledc_channel_config_t ledc_channel;
	ledc_channel.duty 		= 0;
	ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
	ledc_channel.timer_sel	= TimerIndex;
	ledc_channel.channel	= PWMChannel;
	ledc_channel.gpio_num	= GPIO;
	ledc_channel.hpoint		= 0;
	ledc_channel.intr_type	= LEDC_INTR_FADE_END;

	::ledc_channel_config(&ledc_channel);
	::ledc_fade_func_install(0);
 }

 /**
  * @brief Set PWM channel data
  *
  * @param [in] PWMChannel 	LEDC Channel [0..7]
  * @param [out] value 			Current PWM Duty [0..255]
  */

uint8_t GPIO::PWMValue(ledc_channel_t PWMChannel) {
	if (PWMValuesCache.count(PWMChannel)) {
		if (PWMValuesCache[PWMChannel].Updated + (uint32_t)floor(PWM_FADING_LENGTH / 1000) < Time::Uptime())
			PWMValuesCache.erase(PWMChannel);
		else
			return PWMValuesCache[PWMChannel].Value;
	}

	int Duty = ledc_get_duty(LEDC_HIGH_SPEED_MODE, PWMChannel);
	return floor(Duty);
}

 /**
  * @brief Set PWM channel data
  *
  * @param [in] PWMChannel 	LEDC Channel [0..7]
  * @param [in] Duty 				Channel Power [0..255]
  */



void GPIO::PWMFadeTo(ledc_channel_t PWMChannel, uint8_t Duty, uint16_t FadeTime) {
	if (PWMChannel == LEDC_CHANNEL_MAX)
		return;

	if (FadeTime < 50) {
		::ledc_set_duty(LEDC_HIGH_SPEED_MODE, PWMChannel, Duty);
        ::ledc_update_duty(LEDC_HIGH_SPEED_MODE, PWMChannel);
		return;
	}

	if (ESP_OK == ::ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, PWMChannel, Duty, FadeTime))
		if (ESP_OK == ::ledc_fade_start(LEDC_HIGH_SPEED_MODE, PWMChannel, LEDC_FADE_NO_WAIT))
			PWMValuesCache[PWMChannel] = PWMCacheItem(Duty, Time::Uptime());
}

void GPIO::PWMFadeTo(Settings_t::GPIOData_t::Color_t::Item_t Color, uint8_t Duty, uint16_t FadeTime) {
	if (Color.GPIO != GPIO_NUM_0)
		GPIO::PWMFadeTo(Color.Channel, Duty, FadeTime);
}
