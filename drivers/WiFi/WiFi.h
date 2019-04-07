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

#include "_mDNS.h"
//#include "mDNS.h"

#include "Memory.h"
#include "Converter.h"
#include "FreeRTOSWrapper.h"
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

		/**
		 * @brief Get channel.
		 * @return the channel num.
		 */
		uint8_t getChannel() {
			return m_channel;
		}

		string toString();

	private:
		uint8_t 			m_bssid[6];
		int8_t 				m_rssi;
		string 				m_ssid;
		uint8_t				m_channel;
		wifi_auth_mode_t 	m_authMode;
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
		uint32_t      		ip;
		uint32_t      		gw;
		uint32_t            Netmask;
		WiFiEventHandler*	m_pWifiEventHandler;
		uint8_t             m_dnsCount = 0;
		bool                m_eventLoopStarted;
		bool                m_initCalled;
		bool				m_WiFiRunning;
		uint8_t             m_apConnectionStatus;   // ESP_OK = we are connected to an access point.  Otherwise receives wifi_err_reason_t.

		FreeRTOS::Semaphore m_connectFinished 	= FreeRTOS::Semaphore("ConnectFinished");
		FreeRTOS::Semaphore m_scanFinished 		= FreeRTOS::Semaphore("ScanFinished");

		static esp_err_t    eventHandler(void* ctx, system_event_t* event);
		static string		STAHostName;

	public:
		void                Init();
		void 				Stop();

		WiFi_t();
		void 				AddDNSServer(string ip);
		void 				Dump();

		static string 		GetApMac();
		static string 		GetApSSID();
		static uint8_t		GetAPClientsCount();

		static string 		GetMode();
		static string 		GetStaMac();
		static string 		GetStaSSID();
		static string 		GetSSID();

		static void			SetSTAHostname(string);
		static tcpip_adapter_ip_info_t getApIpInfo();
		static tcpip_adapter_ip_info_t getStaIpInfo();

		vector<WiFiAPRecord> Scan();
		uint8_t ConnectAP(const string& SSID, const string& Password, const uint8_t& Channel = 0, bool WaitForConnection = true);
	    void 	StartAP	 (const string& SSID, const string& Password, wifi_auth_mode_t Auth = WIFI_AUTH_WPA2_PSK, uint8_t Channel = 0, bool SSIDIsHidden = false, uint8_t MaxConnections = 16);

		void	SetIPInfo(const string& ip, const string& gw, const string& netmask);
		void	SetIPInfo(const char* ip, const char* gw, const char* netmask);
		void	SetIPInfo(uint32_t ip, uint32_t gw, uint32_t netmask);

	    void 	SetWiFiEventHandler(WiFiEventHandler *WiFiEventHandler);

	    uint8_t	GetConnectionStatus() 	{ return m_apConnectionStatus; }
	    bool	IsRunning() 			{ return m_WiFiRunning; }
};

#endif /* DRIVERS_WIFI_H_ */
