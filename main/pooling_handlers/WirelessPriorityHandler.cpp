#include "HandlersPooling.h"

void HandlersPooling_t::WirelessPriorityHandler::Pool() {
	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	if (PowerManagement::GetWirelessPriority() == ESP_COEX_PREFER_WIFI)
		return;

	if (PowerManagement::GetWirelessPriority() == ESP_COEX_PREFER_BALANCE
		&& (Time::UptimeU() - PowerManagement::GetWirelessPriorityChangeTime()) > 9 * 1000000) {
		PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_WIFI);
	}
}