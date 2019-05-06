/*
 * WiFi.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */
#include "WiFi.h"

#include <esp_heap_trace.h>

static char 	tag[]				= "WiFi";

string WiFi_t::STAHostName = "LOOK.in Device";

/*
static void setDNSServer(char *ip) {
	ip_addr_t dnsserver;
	ESP_LOGD(tag, "Setting DNS[%d] to %s", 0, ip);
	inet_pton(AF_INET, ip, &dnsserver);
	ESP_LOGD(tag, "ip of DNS is %.8x", *(uint32_t *)&dnsserver);
	dns_setserver(0, &dnsserver);
}
*/

WiFi_t::WiFi_t() : ip(0), gw(0), Netmask(0), m_pWifiEventHandler(nullptr) {
	m_eventLoopStarted  = false;
	m_initCalled        = false;
	m_WiFiRunning		= false;
	//m_pWifiEventHandler = new WiFiEventHandler();
	m_apConnectionStatus= UINT8_MAX;    // Are we connected to an access point?
}

/**
 * @brief Primary event handler interface.
 */
esp_err_t WiFi_t::eventHandler(void* ctx, system_event_t* event) {
	// This is the common event handler that we have provided for all event processing.  It is called for every event
	// that is received by the WiFi subsystem.  The "ctx" parameter is an instance of the current WiFi object that we are
	// processing.  We can then retrieve the specific/custom event handler from within it and invoke that.  This then makes this
	// an indirection vector to the real caller.

	WiFi_t *pWiFi = (WiFi_t *)ctx;   // retrieve the WiFi object from the passed in context.

	// If the event we received indicates that we now have an IP address or that a connection was disconnected then unlock the mutex that
	// indicates we are waiting for a connection complete.

	if (event->event_id == SYSTEM_EVENT_STA_START) {
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_START");
		::tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, STAHostName.c_str());
		pWiFi->m_WiFiRunning = true;
	}

	if (event->event_id == SYSTEM_EVENT_STA_STOP) {
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_STOP");
		pWiFi->m_WiFiRunning = false;
	}

	if (event->event_id == SYSTEM_EVENT_STA_CONNECTED) {
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_CONNECTED");
		pWiFi->m_apConnectionStatus = ESP_OK;
		pWiFi->m_WiFiRunning = true;

		pWiFi->m_connectFinished.Give();
	}

	if (event->event_id == SYSTEM_EVENT_STA_GOT_IP || event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {

		if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) { // If we connected and have an IP, change the status to ESP_OK.  Otherwise, change it to the reason code.
			ESP_LOGI(tag, "SYSTEM_EVENT_STA_GOT_IP");
			pWiFi->m_apConnectionStatus = ESP_OK;
			pWiFi->m_WiFiRunning = true;

			pWiFi->m_connectFinished.Give();
		}
		else
		{
			ESP_LOGI(tag, "SYSTEM_EVENT_STA_DISCONNECTED, Reason: %d", event->event_info.disconnected.reason);

			pWiFi->m_apConnectionStatus = event->event_info.disconnected.reason;
			pWiFi->m_WiFiRunning = false;
		}
	}

	if (event->event_id == SYSTEM_EVENT_AP_START) {
		ESP_LOGI(tag, "SYSTEM_EVENT_AP_START");
		pWiFi->m_WiFiRunning = true;
	}

	if (event->event_id == SYSTEM_EVENT_AP_STOP) {
		ESP_LOGI(tag, "SYSTEM_EVENT_AP_STOP");
		pWiFi->m_WiFiRunning = false;
	}


	if (event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_DISCONNECTED");
		pWiFi->m_WiFiRunning = false;
	}

	if (event->event_id == SYSTEM_EVENT_SCAN_DONE)
	{
		ESP_LOGI(tag, "SYSTEM_EVENT_SCAN_DONE");
		::esp_wifi_disconnect();
		pWiFi->m_scanFinished.Give();
	}

	// Invoke the event handler.
	esp_err_t rc;
	if (pWiFi->m_pWifiEventHandler != nullptr)
		rc = pWiFi->m_pWifiEventHandler->getEventHandler()(pWiFi->m_pWifiEventHandler, event);
	else
		rc = ESP_OK;

	return rc;
} // eventHandler

/**
 * @brief Initialize WiFi.
 */
void WiFi_t::Init() {
	// If we have already started the event loop, then change the handler otherwise
	// start the event loop.
	if (m_eventLoopStarted) {
		esp_event_loop_set_cb(WiFi_t::eventHandler, this);   // Returns the old handler.
	}
	else {
		esp_err_t errRc = ::esp_event_loop_init(WiFi_t::eventHandler, this);  // Initialze the event handler.
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_event_loop_init: rc=%d %s", errRc, Converter::ErrorToString(errRc));
			abort();
		}
		m_eventLoopStarted = true;
	}

	if (!m_initCalled) {
		NVS::Init();
		::tcpip_adapter_init();

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		esp_err_t errRc = ::esp_wifi_init(&cfg);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_wifi_init: rc=%d %s", errRc, Converter::ErrorToString(errRc));
			abort();
		}

		::esp_wifi_set_ps(WIFI_PS_NONE);

		errRc = ::esp_wifi_set_storage(WIFI_STORAGE_RAM);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_wifi_set_storage: rc=%d %s", errRc, Converter::ErrorToString(errRc));
			abort();
		}
	}
	m_initCalled = true;
} // Init

void WiFi_t::Stop() {
	::esp_wifi_stop();
	m_WiFiRunning = false;
}

/**
 * @brief Add a reference to a DNS server.
 *
 * Here we define a server that will act as a DNS server.  We can add two DNS
 * servers in total.  The first will be the primary, the second will be the backup.
 * The public Google DNS servers are "8.8.8.8" and "8.8.4.4".
 *
 * For example:
 *
 * @code{.cpp}
 * wifi.addDNSServer("8.8.8.8");
 * wifi.addDNSServer("8.8.4.4");
 * @endcode
 *
 * @param [in] ip The IP address of the DNS Server.
 * @return N/A.
 */
void WiFi_t::AddDNSServer(string ip) {
	ip_addr_t dnsserver;
	ESP_LOGD(tag, "Setting DNS[%d] to %s", m_dnsCount, ip.c_str());
	inet_pton(AF_INET, ip.c_str(), &dnsserver);
	::dns_setserver(m_dnsCount, &dnsserver);
	m_dnsCount++;
} // addDNSServer


/**
 * @brief Perform a WiFi scan looking for access points.
 *
 * An access point scan is performed and a vector of WiFi access point records
 * is built and returned with one record per found scan instance.  The scan is
 * performed in a blocking fashion and will not return until the set of scanned
 * access points has been built.
 *
 * @return A vector of WiFiAPRecord instances.
 */
vector<WiFiAPRecord> WiFi_t::Scan() {
	ESP_LOGD(tag, ">> scan");
	std::vector<WiFiAPRecord> apRecords;

	m_WiFiRunning = true;

	Init();

	esp_err_t errRc = ::esp_wifi_set_mode(WIFI_MODE_STA);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_mode: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	errRc = ::esp_wifi_start();
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_start: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	wifi_scan_config_t conf;
	memset(&conf, 0, sizeof(conf));
	conf.show_hidden = true;
	conf.scan_type = WIFI_SCAN_TYPE_ACTIVE;
	conf.scan_time.active.min = 120;
	conf.scan_time.active.max = 250;
	conf.channel = 0;

	m_scanFinished.Take("ScanFinished");

	esp_err_t rc = ::esp_wifi_scan_start(&conf, true);
	if (rc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_scan_start: %d", rc);
		return apRecords;
	}

	m_scanFinished.Wait();

	uint16_t apCount;  // Number of access points available.
	rc = ::esp_wifi_scan_get_ap_num(&apCount);
	ESP_LOGD(tag, "Count of found access points: %d", apCount);

	wifi_ap_record_t* list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
	if (list == nullptr) {
		ESP_LOGE(tag, "Failed to allocate memory");
		return apRecords;
	}

	errRc = ::esp_wifi_scan_get_ap_records(&apCount, list);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_scan_get_ap_records: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	for (auto i=0; i<apCount; i++) {
		WiFiAPRecord wifiAPRecord;
		memcpy(wifiAPRecord.m_bssid, list[i].bssid, 6);

		wifiAPRecord.m_ssid     = string((char *)list[i].ssid);
		wifiAPRecord.m_authMode = list[i].authmode;
		wifiAPRecord.m_rssi     = list[i].rssi;
		wifiAPRecord.m_channel	= list[i].primary;

		if (wifiAPRecord.m_ssid == "")
			continue;

		bool IsExist = false;
		for (int i=0; i < apRecords.size(); i++) {
			if (apRecords[i].m_ssid == wifiAPRecord.m_ssid) {
				IsExist = true;

				if (apRecords[i].m_rssi <= wifiAPRecord.m_rssi)
					apRecords[i] = wifiAPRecord;
				else
					continue;
			}
		}

		if (!IsExist)
			apRecords.push_back(wifiAPRecord);
	}

	free(list);   // Release the storage allocated to hold the records.

	::esp_wifi_disconnect();
	::esp_wifi_stop();

	m_WiFiRunning = false;

	std::sort(apRecords.begin(),
		apRecords.end(),
		[](const WiFiAPRecord& lhs,const WiFiAPRecord& rhs){ return lhs.m_rssi> rhs.m_rssi;});
	return apRecords;

} // scan

/**
 * @brief Connect to an external access point.
 *
 * The event handler will be called back with the outcome of the connection.
 *
 * @param [in] ssid The network SSID of the access point to which we wish to connect.
 * @param [in] password The password of the access point to which we wish to connect.
 * @param [in] waitForConnection Block until the connection has an outcome.
 * @returns ESP_OK if successfully receives a SYSTEM_EVENT_STA_GOT_IP event.  Otherwise returns wifi_err_reason_t - use GeneralUtils::wifiErrorToString(uint8_t errCode) to print the error.
 */
uint8_t WiFi_t::ConnectAP(const std::string& SSID, const std::string& Password, const uint8_t& Channel, bool WaitForConnection) {
	ESP_LOGD(tag, ">> connectAP: %s %s", SSID.c_str(), Password.c_str());

	if (GetMode() == WIFI_MODE_STA_STR && m_apConnectionStatus == ESP_OK) {
		::esp_wifi_disconnect();
	}

	esp_err_t errRc = ::esp_wifi_stop();
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_stop error: rc=%d %s", errRc, Converter::ErrorToString(errRc));
	}

	//FreeRTOS::Sleep(1000);

	m_WiFiRunning = true;

	m_apConnectionStatus = UINT8_MAX;
	Init();

	if (ip != 0 && gw != 0 && Netmask != 0) {
		::tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client

		tcpip_adapter_ip_info_t ipInfo;
		ipInfo.ip.addr = ip;
		ipInfo.gw.addr = gw;
		ipInfo.netmask.addr = Netmask;

		::tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	}

	errRc = ::esp_wifi_set_mode(WIFI_MODE_STA);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_mode: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	wifi_config_t sta_config;
	::memset(&sta_config, 0, sizeof(sta_config));
	::memcpy(sta_config.sta.ssid, SSID.data(), SSID.size());
	::memcpy(sta_config.sta.password, Password.data(), Password.size());

	sta_config.sta.scan_method = WIFI_FAST_SCAN;
	sta_config.sta.bssid_set = 0;

	if (Channel > 0)
		sta_config.sta.channel = Channel;

	errRc = ::esp_wifi_set_config(WIFI_IF_STA, &sta_config);

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_config: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	//ESP_ERROR_CHECK(::esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));

	errRc = ::esp_wifi_start();
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_start: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	ESP_LOGD(tag, "<< connectAP");
	return m_apConnectionStatus;  // Return ESP_OK if we are now connected and wifi_err_reason_t if not.
} // connectAP


/**
 * @brief Start being an access point.
 *
 * @param[in] SSID The SSID to use to advertize for stations.
 * @param[in] Password The password to use for station connections.
 * @param[in] Auth The authorization mode for access to this access point.  Options are:
 * * WIFI_AUTH_OPEN
 * * WIFI_AUTH_WPA_PSK
 * * WIFI_AUTH_WPA2_PSK
 * * WIFI_AUTH_WPA_WPA2_PSK
 * * WIFI_AUTH_WPA2_ENTERPRISE
 * * WIFI_AUTH_WEP
 * @param[in] Channel from the access point.
 * @param[in] is the ssid hidden or not.
 * @param[in] limiting number of clients.
 * @return N/A.
 */

void WiFi_t::StartAP(const std::string& SSID, const std::string& Password, wifi_auth_mode_t Auth, uint8_t Channel, bool SSIDIsHidden, uint8_t MaxConnections) {
	ESP_LOGD(tag, ">> startAP: ssid: %s", SSID.c_str());

    m_connectFinished.Give();
	if (GetMode() == WIFI_MODE_STA_STR)
		::esp_wifi_disconnect();

	m_WiFiRunning = true;
	Init();

	esp_err_t errRc = ::esp_wifi_set_mode(WIFI_MODE_AP);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_mode: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	// Build the apConfig structure.
	wifi_config_t apConfig;
	::memset(&apConfig, 0, sizeof(apConfig));
	::memcpy(apConfig.ap.ssid, SSID.data(), SSID.size());
	apConfig.ap.ssid_len 		= SSID.size();
	::memcpy(apConfig.ap.password, Password.data(), Password.size());
	apConfig.ap.channel         = Channel;
	apConfig.ap.authmode        = Auth;
	apConfig.ap.ssid_hidden     = (uint8_t) SSIDIsHidden;
	apConfig.ap.max_connection  = MaxConnections;
	apConfig.ap.beacon_interval = 100;

	errRc = ::esp_wifi_set_config(WIFI_IF_AP, &apConfig);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_config: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	errRc = ::esp_wifi_start();
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_start: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	errRc = tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "tcpip_adapter_dhcps_start: rc=%d %s", errRc, Converter::ErrorToString(errRc));
	}

	ESP_LOGD(tag, "<< startAP");

} // startAP

/**
 * @brief Dump diagnostics to the log.
 */
void WiFi_t::Dump() {
	ESP_LOGD(tag, "WiFi Dump");
	ESP_LOGD(tag, "---------");
	char ipAddrStr[30];
	ip_addr_t ip = ::dns_getserver(0);
	inet_ntop(AF_INET, &ip, ipAddrStr, sizeof(ipAddrStr));
	ESP_LOGD(tag, "DNS Server[0]: %s", ipAddrStr);
} // dump

/**
 * @brief Set STA hostname
 */
void WiFi_t::SetSTAHostname(string HostName) {
	STAHostName = HostName;
}
/**
 * @brief Get the AP IP Info.
 * @return The AP IP Info.
 */
tcpip_adapter_ip_info_t WiFi_t::getApIpInfo() {
	tcpip_adapter_ip_info_t ipInfo;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo);
	return ipInfo;
} // getApIpInfo

/**
 * @brief Get the MAC address of the AP interface.
 * @return The MAC address of the AP interface.
 */
string WiFi_t::GetApMac() {
	uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_AP, mac);
	stringstream s;
	s << hex << setfill('0') << setw(2) << (int) mac[0] << ':' << (int) mac[1] << ':' << (int) mac[2] << ':' << (int) mac[3] << ':' << (int) mac[4] << ':' << (int) mac[5];
	return s.str();
} // getApMac


/**
 * @brief Get the AP SSID.
 * @return The AP SSID.
 */
string WiFi_t::GetApSSID() {
	wifi_config_t conf;
	esp_wifi_get_config(WIFI_IF_AP, &conf);
	return string((char *)conf.sta.ssid);
} // getApSSID

/**
 * @brief Get number of clients connected to AP
 * @return The number of clients connected to AP.
 */
uint8_t	WiFi_t::GetAPClientsCount() {
	wifi_sta_list_t Stations;
	esp_err_t Err = esp_wifi_ap_get_sta_list(&Stations);

	if (Err != ESP_OK)
		return 0;

	return Stations.num;
}

/**
 * @brief Get the WiFi Mode.
 * @return The WiFi Mode.
 */
string WiFi_t::GetMode() {
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);
	switch(mode) {
		case WIFI_MODE_NULL	: return WIFI_MODE_NULL_STR;
		case WIFI_MODE_STA	: return WIFI_MODE_STA_STR;
		case WIFI_MODE_AP	: return WIFI_MODE_AP_STR;
		case WIFI_MODE_APSTA: return WIFI_MODE_APSTA_STR;
		default				: return WIFI_MODE_UNKNOWN_STR;
	}
} // getMode


/**
 * @brief Get the STA IP Info.
 * @return The STA IP Info.
 */
tcpip_adapter_ip_info_t WiFi_t::getStaIpInfo() {
	tcpip_adapter_ip_info_t ipInfo;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	return ipInfo;
} // getStaIpInfo


/**
 * @brief Get the MAC address of the STA interface.
 * @return The MAC address of the STA interface.
 */
string WiFi_t::GetStaMac() {
	uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_STA, mac);
	stringstream s;
	s << hex << setfill('0') << setw(2) << (int) mac[0] << ':' << (int) mac[1] << ':' << (int) mac[2] << ':' << (int) mac[3] << ':' << (int) mac[4] << ':' << (int) mac[5];
	return s.str();
} // getStaMac


/**
 * @brief Get the STA SSID.
 * @return The STA SSID.
 */
string WiFi_t::GetStaSSID() {
	wifi_config_t conf;
	esp_wifi_get_config(WIFI_IF_STA, &conf);
	return string((char *)conf.ap.ssid);
} // getStaSSID

/**
 * @brief Get the connected WiFi SSID.
 * @return The SSID.
 */
string WiFi_t::GetSSID() {
	string Mode = WiFi_t::GetMode();

	if (Mode == WIFI_MODE_STA_STR) 	return GetStaSSID();
	if (Mode == WIFI_MODE_AP_STR)	return GetApSSID();

	return "";
} // getSSID

/**
 * @brief Set the IP info and enable DHCP if ip != 0. If called with ip == 0 then DHCP is enabled.
 * If called with bad values it will do nothing.
 *
 * Do not call this method if we are being an access point ourselves.
 *
 * For example, prior to calling `connectAP()` we could invoke:
 *
 * @code{.cpp}
 * myWifi.setIPInfo("192.168.1.99", "192.168.1.1", "255.255.255.0");
 * @endcode
 *
 * @param [in] ip IP address value.
 * @param [in] gw Gateway value.
 * @param [in] netmask Netmask value.
 * @return N/A.
 */
void WiFi_t::SetIPInfo(const std::string& ip, const std::string& gw, const std::string& netmask) {
	SetIPInfo(ip.c_str(), gw.c_str(), netmask.c_str());
} // setIPInfo

void WiFi_t::SetIPInfo(const char* ip, const char* gw, const char* netmask) {
	uint32_t new_ip;
	uint32_t new_gw;
	uint32_t new_netmask;

	auto success = (bool)inet_pton(AF_INET, ip, &new_ip);
	success = success && inet_pton(AF_INET, gw, &new_gw);
	success = success && inet_pton(AF_INET, netmask, &new_netmask);

	if(!success) {
		return;
	}

	SetIPInfo(new_ip, new_gw, new_netmask);
} // SetIPInfo


/**
 * @brief Set the IP Info based on the IP address, gateway and netmask.
 * @param [in] ip The IP address of our ESP32.
 * @param [in] gw The gateway we should use.
 * @param [in] netmask Our TCP/IP netmask value.
 */
void WiFi_t::SetIPInfo(uint32_t ip, uint32_t gw, uint32_t netmask) {
	Init();

	this->ip      = ip;
	this->gw      = gw;
	this->Netmask = netmask;

	if(ip != 0 && gw != 0 && netmask != 0) {
		tcpip_adapter_ip_info_t ipInfo;
		ipInfo.ip.addr      = ip;
		ipInfo.gw.addr      = gw;
		ipInfo.netmask.addr = netmask;
		::tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
		::tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
	}
	else {
		ip = 0;
		::tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
	}
} // setIPInfo

/**
 * @brief Return a string representation of the WiFi access point record.
 *
 * @return A string representation of the WiFi access point record.
 */
string WiFiAPRecord::toString() {
	string auth;
	switch(getAuthMode()) {
	case WIFI_AUTH_OPEN:
		auth = "WIFI_AUTH_OPEN";
		break;
	case WIFI_AUTH_WEP:
		auth = "WIFI_AUTH_WEP";
		break;
	case WIFI_AUTH_WPA_PSK:
		auth = "WIFI_AUTH_WPA_PSK";
		break;
	case WIFI_AUTH_WPA2_PSK:
		auth = "WIFI_AUTH_WPA2_PSK";
		break;
	case WIFI_AUTH_WPA_WPA2_PSK:
		auth = "WIFI_AUTH_WPA_WPA2_PSK";
		break;
	default:
		auth = "<unknown>";
		break;
	}
	stringstream s;
	s<< "ssid: " << m_ssid << ", auth: " << auth << ", rssi: " << m_rssi;
	return s.str();
} // toString

/**
 * @brief Set the event handler to use to process detected events.
 * @param[in] WiFiEventHandler The class that will be used to process events.
 */
void WiFi_t::SetWiFiEventHandler(WiFiEventHandler* WiFiEventHandler) {
	ESP_LOGD(tag, ">> setWiFiEventHandler: 0x%d", (uint32_t)WiFiEventHandler);
	this->m_pWifiEventHandler = WiFiEventHandler;
	ESP_LOGD(tag, "<< setWiFiEventHandler");
} // setWiFiEventHandler
