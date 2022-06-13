#ifndef WIRELESSPRIORITY_HANDLER
#define WIRELESSPRIORITY_HANDLER

#include "Globals.h"

class WirelessPriorityHandler {
	public:
		static void 	Pool();
};

void WirelessPriorityHandler::Pool() {
	if (WiFi.GetMode() != WIFI_MODE_STA_STR)
		return;

	if (PowerManagement::GetWirelessPriority() == ESP_COEX_PREFER_WIFI)
		return;

	if (PowerManagement::GetWirelessPriority() == ESP_COEX_PREFER_BALANCE
		&& (Time::UptimeU() - PowerManagement::GetWirelessPriorityChangeTime()) > 9 * 1000000) {
		PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_WIFI);
	}
}

#endif
