/*
 * WiFiEventHandler.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */

#include "WiFiEventHandler.h"
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>
#include "sdkconfig.h"

static char tag[] = "WiFiEventHandler";

/**
 * @brief The entry point into the event handler.
 * Examine the event passed into the handler controller by the WiFi subsystem and invoke
 * the corresponding handler.
 * @param [in] ctx
 * @param [in] event
 * @return ESP_OK if the event was handled otherwise an error.
 */

void WiFiEventHandler::EventHandler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if (event_handler_arg == nullptr) {
		ESP_LOGD(tag, "No context");
		return;
	}

	WiFiEventHandler *pWiFiEventHandler = (WiFiEventHandler *)event_handler_arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		::esp_wifi_connect();
		pWiFiEventHandler->staStart();
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP)
		pWiFiEventHandler->staStop();

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
		pWiFiEventHandler->staConnected();

	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* IPInfo = (ip_event_got_ip_t*) event_data;
		pWiFiEventHandler->staGotIPv4(*IPInfo);
	}

	if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {
		ip_event_got_ip6_t* IPInfo = (ip_event_got_ip6_t*) event_data;
		pWiFiEventHandler->staGotIPv6(*IPInfo);
	}

	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
		pWiFiEventHandler->staLostIp();
	}

    if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
		wifi_event_sta_disconnected_t* DisconnectedInfo = (wifi_event_sta_disconnected_t*) event_data;
		pWiFiEventHandler->staDisconnected(*DisconnectedInfo);
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
		pWiFiEventHandler->apStart();

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP)
		pWiFiEventHandler->apStop();

	//if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_SCAN_DONE)


	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_WIFI_READY)
		pWiFiEventHandler->wifiReady();


	if (pWiFiEventHandler->nextHandler != nullptr) {
		printf("Found a next handler\n");
		EventHandler(pWiFiEventHandler->nextHandler, event_base, event_id, event_data);
	} else {
		//printf("NOT Found a next handler\n");
	}

	pWiFiEventHandler->Generic(event_handler_arg, event_base, event_id, event_data);
}

WiFiEventHandler::WiFiEventHandler() {}


/**
 * @brief Get the event handler.
 * Retrieve the event handler function to be passed to the ESP-IDF event handler system.
 * @return The event handler function.
 */
esp_event_handler_t WiFiEventHandler::getEventHandler() {
	return EventHandler;
} // getEventHandler


/**
 * @brief Handle the Station Got IP event.
 * Handle having received/assigned an IP address when we are a station.
 * @param [in] event_sta_got_ip The Station Got IP event.
 * @return An indication of whether or not we processed the event successfully.
 */
esp_err_t WiFiEventHandler::staGotIPv4(ip_event_got_ip_t GotIPv4Info) {
	ESP_LOGD(tag, "default staGotIPv4");
	return ESP_OK;
} // staGotIPv4

/**
 * @brief Handle the Station Got IPv6 event.
 * Handle having received/assigned an IP address when we are a station.
 * @param [in] event_sta_got_ip The Station Got IP event.
 * @return An indication of whether or not we processed the event successfully.
 */
esp_err_t WiFiEventHandler::staGotIPv6(ip_event_got_ip6_t GotIPv6Info) {
	ESP_LOGD(tag, "default staGotIPv6");
	return ESP_OK;
} // staGotIPv6

/**
 * @brief Handle the Access Point started event.
 * Handle an indication that the ESP32 has started being an access point.
 * @return An indication of whether or not we processed the event successfully.
 */
esp_err_t WiFiEventHandler::apStart() {
	ESP_LOGD(tag, "default apStart");
	return ESP_OK;
} // apStart

/**
 * @brief Handle the Access Point stop event.
 * Handle an indication that the ESP32 has stopped being an access point.
 * @return An indication of whether or not we processed the event successfully.
 */
esp_err_t WiFiEventHandler::apStop() {
	ESP_LOGD(tag, "default apStop");
	return ESP_OK;
} // apStop

esp_err_t WiFiEventHandler::wifiReady() {
	ESP_LOGD(tag, "default wifiReady");
	return ESP_OK;
} // wifiReady

esp_err_t WiFiEventHandler::staStart() {
	ESP_LOGD(tag, "default staStart");
	return ESP_OK;
} // staStart

esp_err_t WiFiEventHandler::staStop() {
	ESP_LOGD(tag, "default staStop");
	return ESP_OK;
} // staStop

esp_err_t WiFiEventHandler::staConnected() {
	ESP_LOGD(tag, "default staConnected");
	return ESP_OK;
} // staConnected

esp_err_t WiFiEventHandler::staDisconnected(system_event_sta_disconnected_t DisconnectedInfo) {
	ESP_LOGD(tag, "default staDisconnected");
	return ESP_OK;
} // staDisconnected

esp_err_t WiFiEventHandler::apStaConnected() {
	ESP_LOGD(tag, "default apStaConnected");
	return ESP_OK;
} // apStaConnected

esp_err_t WiFiEventHandler::apStaDisconnected() {
	ESP_LOGD(tag, "default apStaDisconnected");
	return ESP_OK;
} // apStaDisconnected

esp_err_t WiFiEventHandler::ConnectionTimeout() {
	ESP_LOGD(tag, "default ConnectionTimeout");
	return ESP_OK;
} // ConnectionTimeout

esp_err_t WiFiEventHandler::staLostIp() {
	ESP_LOGD(tag, "default staLostIp");
	return ESP_OK;
} // staLostIp


void WiFiEventHandler::Generic(void * arg, esp_event_base_t eventBase, int32_t eventId, void * eventData) {
}


WiFiEventHandler::~WiFiEventHandler() {
	if (nextHandler != nullptr) {
		delete nextHandler;
	}
} // ~WiFiEventHandler
