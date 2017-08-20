/*
 * WiFi.h
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_
#include "sdkconfig.h"
#if defined(CONFIG_WIFI_ENABLED)
#include <string>
#include <vector>
#include <mdns.h>
#include "WiFiEventHandler.h"

using namespace std;

#define WIFI_MODE_NULL_STR 		"WIFI_MODE_NULL"
#define WIFI_MODE_STA_STR  		"WIFI_MODE_STA"
#define WIFI_MODE_AP_STR 			"WIFI_MODE_AP"
#define WIFI_MODE_APSTA_STR  	"WIFI_MODE_APSTA"
#define WIFI_MODE_UNKNOWN_STR "WIFI_MODE_UNKNOWN"

/**
 * @brief Manage mDNS server.
 */
class MDNS {
public:
	MDNS();
	~MDNS();
	void serviceAdd(string service, string proto, uint16_t port);
	void serviceInstanceSet(string service, string proto, string instance);
	void servicePortSet(string service, string proto, uint16_t port);
	void serviceRemove(string service, string proto);
	void setHostname(string hostname);
	void setInstance(string instance);
private:
	mdns_server_t *m_mdns_server = nullptr;
};

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
	string      	ip;
	string      	gw;
	string      	netmask;
	WiFiEventHandler 	*wifiEventHandler;

public:
	WiFi_t();
	void addDNSServer(string ip);
	void dump();
	static string getApMac();
	static tcpip_adapter_ip_info_t getApIpInfo();
	static string getApSSID();
	static string getMode();
	static tcpip_adapter_ip_info_t getStaIpInfo();
	static string getStaMac();
	static string getStaSSID();

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

#endif // CONFIG_WIFI_ENABLED
#endif /* MAIN_WIFI_H_ */
