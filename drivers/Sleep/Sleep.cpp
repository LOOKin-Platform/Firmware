/*
*    Sleep.cpp
*    Class describes SoC sleep modes
*
*/

#include "Sleep.h"
#include "DateTime.h"

static char tag[] = "Sleep";

static uint32_t WakeUpTime = Time::Uptime();

Sleep::Sleep() {
}

esp_sleep_wakeup_cause_t Sleep::GetWakeUpReason() {
	return esp_sleep_get_wakeup_cause();
	/*
	ESP_SLEEP_WAKEUP_UNDEFINED
	ESP_SLEEP_WAKEUP_EXT0
	ESP_SLEEP_WAKEUP_EXT1
	ESP_SLEEP_WAKEUP_TIMER
	ESP_SLEEP_WAKEUP_TOUCHPAD
	ESP_SLEEP_WAKEUP_ULP

	switch (esp_sleep_get_wakeup_cause()) {
		case ESP_SLEEP_WAKEUP_TIMER: {
			ESP_LOGI(tag, "Wake up from timer");
			break;
		}

		case ESP_SLEEP_WAKEUP_UNDEFINED:
		default:
			ESP_LOGI(tag, "Not a sleep reset\n");
	}

	ESP_LOGI(tag, "Uptime - %d", Time::Uptime() - WakeUpTime);
	*/
}

Sleep::~Sleep() {

}

void Sleep::SetTimerInterval(uint16_t Interval) {
    ::esp_sleep_enable_timer_wakeup(Interval * 1000000);
}

void Sleep::SetGPIOWakeupSource(gpio_num_t gpio_num, int level) {
	::esp_sleep_enable_ext0_wakeup(gpio_num, level);
	//rtc_gpio_isolate(gpio_num);
}

void Sleep::LightSleep(uint16_t Interval) {
	// switch off Bluetooth

	BLE::Stop();
	::esp_wifi_stop();

	::esp_deep_sleep_start();
	/*
	if (::esp_light_sleep_start() != ESP_OK) {
		ESP_LOGE(tag, "Error going to light sleep: Bluetooth and Wi-Fi must be switched off");
	}
	else {
		ESP_LOGI(tag, "Going to the light sleep")
	}
	*/

}
