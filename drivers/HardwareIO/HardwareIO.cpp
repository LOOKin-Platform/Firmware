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

void GPIO::SetupPWM(gpio_num_t GPIO, ledc_channel_t PWMChannel, uint16_t Freq, ledc_timer_bit_t Resolution, ledc_timer_t TimerIndex, ledc_clk_cfg_t ClockSource, uint32_t DefaultDuty) {
	if (GPIO == GPIO_NUM_0)
		return;

	if (PWMChannel == LEDC_CHANNEL_MAX)
		return;

	if (!PWMIsInited)
		PWMIsInited = true;

	//::rtc_gpio_isolate(GPIO);

    uint8_t Group = (PWMChannel/8);
    uint8_t Timer = ((PWMChannel/2)%4);

	ledc_timer_config_t ledc_timer;
	::memset(&ledc_timer, 0, sizeof(ledc_timer));

	ledc_timer.duty_resolution	= Resolution;
	ledc_timer.freq_hz			= Freq;
	ledc_timer.speed_mode		= (ledc_mode_t)Group;
	ledc_timer.timer_num		= (TimerIndex == LEDC_TIMER_MAX ? (ledc_timer_t)Timer : TimerIndex);
	ledc_timer.clk_cfg			= ClockSource;//LEDC_USE_REF_TICK;

    if(ledc_timer_config(&ledc_timer) != ESP_OK)
    {
        ESP_LOGE(tag,"LEDC setup failed!");
        return;
    }

   	uint8_t Channel = (PWMChannel%8);

	ledc_channel_config_t ledc_channel;
	::memset(&ledc_channel, 0, sizeof(ledc_channel));

	ledc_channel.speed_mode = (ledc_mode_t)Group;
	ledc_channel.timer_sel	= (TimerIndex == LEDC_TIMER_MAX ? (ledc_timer_t)Timer : TimerIndex);
	ledc_channel.channel	= (ledc_channel_t)Channel;
	ledc_channel.gpio_num	= GPIO;
	ledc_channel.duty 		= DefaultDuty;
	ledc_channel.hpoint		= 0;
	ledc_channel.intr_type	= LEDC_INTR_FADE_END;

    if(::ledc_channel_config(&ledc_channel) != ESP_OK)
    {
        ESP_LOGE(tag,"LEDC ledc_channel_config failed!");
        return;
    }
 }

void GPIO::PWMFadeInstallFunction() {
	if(::ledc_fade_func_install(0) != ESP_OK)
        ESP_LOGE(tag,"LEDC ledc_fade_func_install failed!");
}

 /**
  * @brief Set PWM channel data
  *
  * @param [in] PWMChannel 	LEDC Channel [0..7]
  * @param [out] value 			Current PWM Duty [0..255]
  */

uint32_t GPIO::PWMValue(ledc_channel_t PWMChannel) {
	if (PWMChannel == LEDC_CHANNEL_MAX)
		return 0;

	if (PWMValuesCache.count(PWMChannel)) {
		if (PWMValuesCache[PWMChannel].Updated + (uint32_t)floor(PWM_FADING_LENGTH / 1000) < Time::Uptime())
			PWMValuesCache.erase(PWMChannel);
		else
			return PWMValuesCache[PWMChannel].Value;
	}

#if CONFIG_IDF_TARGET_ESP32
    return ledc_get_duty(LEDC_HIGH_SPEED_MODE, PWMChannel);
#elif CONFIG_IDF_TARGET_ESP32C6
	return ledc_get_duty(LEDC_LOW_SPEED_MODE, PWMChannel);
#endif

}

void GPIO::PWMSetDuty(ledc_channel_t PWMChannel, uint32_t Duty) {
    uint8_t Group=(PWMChannel/8);

    ::ledc_set_duty((ledc_mode_t)Group, PWMChannel, Duty);
    ::ledc_update_duty((ledc_mode_t)Group, PWMChannel);
}


void GPIO::PWMFadeTo(ledc_channel_t PWMChannel, uint32_t Duty, uint16_t FadeTime) {
	if (PWMChannel == LEDC_CHANNEL_MAX)
		return;

	if (FadeTime < 50) {
		PWMSetDuty(PWMChannel, Duty);
		return;
	}

#if CONFIG_IDF_TARGET_ESP32
	if (ESP_OK == ::ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, PWMChannel, Duty, FadeTime))
		if (ESP_OK == ::ledc_fade_start(LEDC_HIGH_SPEED_MODE, PWMChannel, LEDC_FADE_NO_WAIT))
			PWMValuesCache[PWMChannel] = PWMCacheItem(Duty, Time::Uptime());
#elif CONFIG_IDF_TARGET_ESP32C6
	if (ESP_OK == ::ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, PWMChannel, Duty, FadeTime))
		if (ESP_OK == ::ledc_fade_start(LEDC_LOW_SPEED_MODE, PWMChannel, LEDC_FADE_NO_WAIT))
			PWMValuesCache[PWMChannel] = PWMCacheItem(Duty, Time::Uptime());
#endif
}

void GPIO::PWMFadeTo(Settings_t::GPIOData_t::Color_t::Item_t Color, uint32_t Duty, uint16_t FadeTime) {
	if (Color.GPIO != GPIO_NUM_0)
		GPIO::PWMFadeTo(Color.Channel, Duty, FadeTime);
}
