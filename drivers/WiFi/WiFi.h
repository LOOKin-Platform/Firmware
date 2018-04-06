/*
 * WiFi.h
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */

#ifndef DRIVERS_WIFI_H_
#define DRIVERS_WIFI_H_

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <mdns.h>
#include <string.h>

#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "WiFiEventHandler.h"

using namespace std;

#define WIFI_MODE_NULL_STR 		"WIFI_MODE_NULL"
#define WIFI_MODE_STA_STR  		"WIFI_MODE_STA"
#define WIFI_MODE_AP_STR 		"WIFI_MODE_AP"
#define WIFI_MODE_APSTA_STR  	"WIFI_MODE_APSTA"
#define WIFI_MODE_UNKNOWN_STR	"WIFI_MODE_UNKNOWN"

class WiFiAPRecord {
public:
	friend class WiFi_t;

	/**
	 * @brief Get the auth mode.
	 * @return The auth mode.
	 */
	wifi_auth_mode_t getAuthMode() {
		return m_authMode;
	}

	/**
	 * @brief Get the RSSI.
	 * @return the RSSI.
	 */
	int8_t getRSSI() {
		return m_rssi;
	}

	/**
	 * @brief Get the SSID.
	 * @return the SSID.
	 */
	string getSSID() {
		return m_ssid;
	}

	string toString();

private:
	uint8_t m_bssid[6];
	int8_t m_rssi;
	string m_ssid;
	wifi_auth_mode_t m_authMode;
};

/**
 * @brief %WiFi driver.
 *
 * Encapsulate control of %WiFi functions.
 *
 * WiFi wifi;
 * TargetWiFiEventHandler *eventHandler = new TargetWiFiEventHandler();
 * wifi.setWifiEventHandler(eventHandler);
 * wifi.connectAP("myssid", "mypassword");
 * @endcode
 */

class WiFi_t {
private:
	string      			ip;
	string      			gw;
	string      			netmask;
	WiFiEventHandler 	*wifiEventHandler;

public:
	WiFi_t();
	void 				addDNSServer(string ip);
	void 				dump();
	static string 		getApMac();
	static string 		getApSSID();
	static string 		getMode();
	static string 		getStaMac();
	static string 		getStaSSID();

	static tcpip_adapter_ip_info_t getApIpInfo();
	static tcpip_adapter_ip_info_t getStaIpInfo();

	vector<WiFiAPRecord> Scan();
	void ConnectAP(string ssid, string passwd);
	void StartAP(string ssid, string passwd);

	void setIPInfo(string ip, string gw, string netmask);

	/**
	 * Set the event handler to use to process detected events.
	 * @param[in] wifiEventHandler The class that will be used to process events.
	 */
	void setWifiEventHandler(WiFiEventHandler *wifiEventHandler) {
		this->wifiEventHandler = wifiEventHandler;
	}
private:
	int m_dnsCount=0;
	//char *m_dnsServer = nullptr;

};

#endif /* DRIVERS_WIFI_H_ */
