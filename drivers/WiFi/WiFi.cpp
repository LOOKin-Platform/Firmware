/*
 * WiFi.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: kolban
 */
#include "WiFi.h"
#include "PowerManagement.h"

static char 	tag[]						= "WiFi";

string			WiFi_t::STAHostName 		= "LOOKin Device";
bool			WiFi_t::m_WiFiNetworkSwitch = false;

esp_netif_t* 	WiFi_t::NetIfSTAHandle		= NULL;
esp_netif_t*	WiFi_t::NetIfAPHandle		= NULL;

esp_event_handler_instance_t WiFi_t::instance_any_id = NULL;
esp_event_handler_instance_t WiFi_t::instance_any_ip = NULL;


WiFi_t::WiFi_t() : ip(0), gw(0), Netmask(0), m_pWifiEventHandler(nullptr) {
	m_initCalled        = false;
	m_WiFiRunning		= false;
	//m_pWifiEventHandler = new WiFiEventHandler();
	m_apConnectionStatus= UINT8_MAX;    // Are we connected to an access point?
}

/**
 * @brief Primary event handler interface.
 */

void WiFi_t::eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	// This is the common event handler that we have provided for all event processing.  It is called for every event
	// that is received by the WiFi subsystem.  The "ctx" parameter is an instance of the current WiFi object that we are
	// processing.  We can then retrieve the specific/custom event handler from within it and invoke that.  This then makes this
	// an indirection vector to the real caller.

	WiFi_t *pWiFi = (WiFi_t *)arg;   // retrieve the WiFi object from the passed in context.

	// If the event we received indicates that we now have an IP address or that a connection was disconnected then unlock the mutex that
	// indicates we are waiting for a connection complete.

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(tag, "WIFI_EVENT_STA_START");
		pWiFi->m_WiFiRunning = true;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
		ESP_LOGI(tag, "WIFI_EVENT_STA_STOP");
		pWiFi->m_WiFiRunning = false;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
		ESP_LOGI(tag, "WIFI_EVENT_STA_CONNECTED");

        esp_netif_create_ip6_linklocal(WiFi_t::GetNetIf());

		::esp_netif_set_hostname(WiFi_t::GetNetIf(), STAHostName.c_str());

		esp_ip_addr_t GoogleDNS;
		inet_pton(AF_INET, "8.8.8.8", &GoogleDNS);
		esp_netif_dns_info_t DNSInfo;
		DNSInfo.ip = GoogleDNS;

		::esp_netif_set_dns_info(WiFi_t::GetNetIf(), ESP_NETIF_DNS_FALLBACK, &DNSInfo);


		pWiFi->m_apConnectionStatus = ESP_OK;
		pWiFi->m_WiFiRunning = true;

		pWiFi->m_connectFinished.Give();
    }

	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ESP_LOGI(tag, "IP_EVENT_STA_GOT_IP");
		pWiFi->m_apConnectionStatus = ESP_OK;
		pWiFi->m_WiFiRunning = true;

		pWiFi->m_connectFinished.Give();
	}

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

		ESP_LOGI(tag, "WIFI_EVENT_STA_DISCONNECTED");

        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
		pWiFi->m_apConnectionStatus = disconnected->reason;
		pWiFi->m_WiFiRunning = false;
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
		ESP_LOGI(tag, "WIFI_EVENT_AP_START");
		pWiFi->m_WiFiRunning = true;
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
		ESP_LOGI(tag, "WIFI_EVENT_AP_STOP");
		pWiFi->m_WiFiRunning = false;
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
	{
		ESP_LOGI(tag, "WIFI_EVENT_SCAN_DONE");
		::esp_wifi_disconnect();
		pWiFi->m_scanFinished.Give();
	}
	// Invoke the event handler.
	if (pWiFi->m_pWifiEventHandler != nullptr) {
		pWiFi->m_pWifiEventHandler->getEventHandler()(pWiFi->m_pWifiEventHandler, event_base, event_id, event_data);

	}
} // eventHandler

bool WiFi_t::IsWiFiInited() {
	wifi_mode_t *mode = {0};
	esp_err_t err = esp_wifi_get_mode(mode);

	return !(err == ESP_ERR_WIFI_NOT_INIT);
}

/**
 * @brief Initialize WiFi.
 */
void WiFi_t::Init() {
	ESP_LOGE(tag,"WiFi_t::Init");

	esp_err_t errRc = ESP_OK;

	if (!m_initCalled) 
	{
		NVS::Init();

		if (!IsExternalInitExists) {
			ESP_ERROR_CHECK(::esp_netif_init());
			ESP_ERROR_CHECK(::esp_event_loop_create_default());
		}
		
		if (!IsWiFiInited()) {
			wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
			errRc = ::esp_wifi_init(&cfg);

			if (errRc != ESP_OK) 
			{
				ESP_LOGE(tag, "esp_wifi_init: rc=%d %s", errRc, Converter::ErrorToString(errRc));
				abort();
			}
		}

		if (!IsExternalInitExists)
		{
			if (NetIfSTAHandle 	== NULL) 
			{
				NetIfSTAHandle = esp_netif_create_default_wifi_sta();
			}
			
			m_WiFiRunning = true;
		}

		if (NetIfAPHandle 	== NULL) NetIfAPHandle 	= esp_netif_create_default_wifi_ap();

		::esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID	, &eventHandler, this, &WiFi_t::instance_any_id);
		::esp_event_handler_instance_register(IP_EVENT	, ESP_EVENT_ANY_ID	, &eventHandler, this, &WiFi_t::instance_any_ip);

		PowerManagement::SetWiFiOptions();

		errRc = ::esp_wifi_set_storage(WIFI_STORAGE_RAM);
		if (errRc != ESP_OK) {
			ESP_LOGE(tag, "esp_wifi_set_storage: rc=%d %s", errRc, Converter::ErrorToString(errRc));
			abort();
		}
	}
	m_initCalled = true;
} // Init

void WiFi_t::DeInit() {
	if (!m_initCalled) return;

	::esp_wifi_stop();

	if (NetIfSTAHandle != NULL) {
		::esp_netif_dhcpc_stop(NetIfSTAHandle);
		::esp_netif_destroy(NetIfSTAHandle);
		NetIfSTAHandle = NULL;
	}

	if (NetIfAPHandle != NULL) {
		::esp_netif_dhcpc_stop(NetIfAPHandle);
		::esp_netif_destroy(NetIfAPHandle);
		NetIfAPHandle = NULL;
	}

    ::esp_event_handler_instance_unregister(WIFI_EVENT	, ESP_EVENT_ANY_ID		, instance_any_id);
    ::esp_event_handler_instance_unregister(IP_EVENT	, ESP_EVENT_ANY_ID		, instance_any_ip);
	::esp_event_loop_delete_default();

	::esp_wifi_restore();

	::esp_netif_deinit();
	::esp_wifi_deinit();

	m_initCalled = false;

}

void WiFi_t::Stop() {
	::esp_wifi_stop();
	::esp_wifi_restore();
	m_WiFiRunning = false;
}

esp_netif_t * WiFi_t::GetNetIf() {
	if (WiFi_t::GetMode() == WIFI_MODE_STA_STR)
		return ::esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	else if (WiFi_t::GetMode() == WIFI_MODE_AP_STR)
		return ::esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
	else
		return nullptr;
}

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

	Init();

	wifi_mode_t CurrentMode = WIFI_MODE_NULL;

	if (m_WiFiRunning)
		::esp_wifi_get_mode(&CurrentMode);
	else
	{
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
	}

	m_WiFiRunning = true;

	wifi_scan_config_t conf;
	memset(&conf, 0, sizeof(conf));
	conf.show_hidden 			= 1;
	conf.scan_type 				= WIFI_SCAN_TYPE_ACTIVE;
	conf.scan_time.active.min 	= 500;
	conf.scan_time.active.max 	= 1000;
	conf.channel = 0;

	m_scanFinished.Take("ScanFinished");

	esp_err_t rc = ::esp_wifi_scan_start(&conf, true);
	if (rc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_scan_start: %d %s", rc, Converter::ErrorToString(rc));
		return apRecords;
	}

	m_scanFinished.Wait();

	uint16_t apCount = 0;  // Number of access points available.
	rc = ::esp_wifi_scan_get_ap_num(&apCount);
	ESP_LOGE(tag, "Count of found access points: %d", apCount);

	wifi_ap_record_t* list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
	if (list == nullptr) {
		ESP_LOGE(tag, "Failed to allocate memory");
		return apRecords;
	}

	esp_err_t ScanErrRc = ::esp_wifi_scan_get_ap_records(&apCount, list);
	if (ScanErrRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_scan_get_ap_records: rc=%d %s", ScanErrRc, Converter::ErrorToString(ScanErrRc));
		abort();
	}

	for (auto i=0; i < apCount; i++) {
		WiFiAPRecord wifiAPRecord;
		memcpy(wifiAPRecord.m_bssid, list[i].bssid, 6);

		wifiAPRecord.m_ssid     = string((char *)list[i].ssid);
		wifiAPRecord.m_authMode = list[i].authmode;
		wifiAPRecord.m_rssi     = list[i].rssi;
		wifiAPRecord.m_channel	= list[i].primary;

		ESP_LOGE("wifiAPRecord.m_ssid", "%s", wifiAPRecord.m_ssid.c_str());

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

	if (CurrentMode != WIFI_MODE_AP) {
		::esp_wifi_disconnect();
		::esp_wifi_stop();
		m_WiFiRunning = false;
	}

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
	ESP_LOGD(tag, "Connecting to AP: %s %s", SSID.c_str(), Password.c_str());

	m_WiFiNetworkSwitch = false;
	IsIPCheckSuccess = false;

	if (GetMode() == WIFI_MODE_STA_STR && m_apConnectionStatus == ESP_OK) {
		::esp_wifi_disconnect();
		m_WiFiNetworkSwitch = true;
		DHCPStop();
		FreeRTOS::Sleep(1000);
	}

	esp_err_t errRc = ::esp_wifi_stop();
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_stop error: rc=%d %s", errRc, Converter::ErrorToString(errRc));
	}

	m_WiFiRunning = true;

	m_apConnectionStatus = UINT8_MAX;
	Init();

	if (ip != 0 && gw != 0 && Netmask != 0) {

		DHCPStop();

		esp_netif_ip_info_t ipInfo;
		ipInfo.ip.addr = ip;
		ipInfo.gw.addr = gw;
		ipInfo.netmask.addr = Netmask;

		::esp_netif_set_ip_info(WiFi_t::GetNetIf(), &ipInfo);
	}

	DHCPStart();

	errRc = ::esp_wifi_set_mode(WIFI_MODE_STA);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_mode: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		m_apConnectionStatus = UINT8_MAX;
		return m_apConnectionStatus;
	}

	wifi_config_t sta_config;
	::memset(&sta_config, 0, sizeof(sta_config));
	::memcpy(sta_config.sta.ssid, SSID.data(), SSID.size());
	::memcpy(sta_config.sta.password, Password.data(), Password.size());

	sta_config.sta.scan_method		= WIFI_ALL_CHANNEL_SCAN;
	sta_config.sta.bssid_set		= 0;
	sta_config.sta.sort_method		= WIFI_CONNECT_AP_BY_SIGNAL;
	sta_config.sta.listen_interval	= 5;

	sta_config.sta.pmf_cfg.capable	= true;
	sta_config.sta.pmf_cfg.required	= false;
	sta_config.sta.rm_enabled		= 1;
	sta_config.sta.btm_enabled		= 1;

	sta_config.sta.channel			= Channel;

	errRc = ::esp_wifi_set_config(WIFI_IF_STA, &sta_config);

	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_config: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		m_apConnectionStatus = UINT8_MAX;
		return m_apConnectionStatus;
	}

	errRc = ::esp_wifi_start();
	if (errRc != ESP_OK)
		ESP_LOGE(tag, "esp_wifi_start: rc=%d %s", errRc, Converter::ErrorToString(errRc));

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

void WiFi_t::StartAP(const std::string& SSID, uint8_t Channel, bool SSIDIsHidden, uint8_t MaxConnections) {
	ESP_LOGD(tag, ">> startAP: ssid: %s", SSID.c_str());

	m_WiFiNetworkSwitch = false;

    m_connectFinished.Give();
	if (GetMode() == WIFI_MODE_STA_STR)
		::esp_wifi_disconnect();

	m_WiFiRunning = true;
	Init();

	esp_err_t errRc;

	if (GetMode() == WIFI_MODE_STA_STR && m_apConnectionStatus == ESP_OK) {
		::esp_wifi_disconnect();
		FreeRTOS::Sleep(1000);
	}

	errRc = ::esp_wifi_stop();
	if (errRc != ESP_OK)
		ESP_LOGE(tag, "esp_wifi_stop error: rc=%d %s", errRc, Converter::ErrorToString(errRc));

	errRc = ::esp_wifi_set_mode(WIFI_MODE_AP);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "esp_wifi_set_mode: rc=%d %s", errRc, Converter::ErrorToString(errRc));
		abort();
	}

	// Build the apConfig structure.
	wifi_config_t apConfig;
	::memset(&apConfig, 0, sizeof(apConfig));
	::memcpy(apConfig.ap.ssid, SSID.data(), SSID.size());
	apConfig.ap.ssid_len 		= SSID.size();
	::memcpy(apConfig.ap.password, "", 0);

	apConfig.ap.channel         = Channel;
	apConfig.ap.authmode        = WIFI_AUTH_OPEN;
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

	ESP_LOGD(tag, "<< startAP");

} // startAP

/**
 * @brief Set STA hostname
 */
void WiFi_t::SetSTAHostname(string HostName) {
	STAHostName = HostName;
}

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
 * @brief Get the IP Info.
 * @return The IP Info.
 */
esp_netif_ip_info_t WiFi_t::GetIPInfo() {
	esp_netif_ip_info_t IPInfo;
	esp_netif_get_ip_info(WiFi_t::GetNetIf(), &IPInfo);
	return IPInfo;
} // GetIPInfo

/**
 * @brief Get if STA in network switch mode
 * @return Is network switching to another one
 */
bool WiFi_t::GetWiFiNetworkSwitch() {
	return m_WiFiNetworkSwitch;
}


void WiFi_t::DHCPStop(uint16_t Pause) {
	esp_netif_dhcp_status_t DHCPStatus;
	::esp_netif_dhcpc_get_status(WiFi_t::GetNetIf(), &DHCPStatus);

	if (DHCPStatus == ESP_NETIF_DHCP_STARTED) {
		::esp_netif_dhcpc_stop(WiFi_t::GetNetIf());

		if (Pause > 0)
			FreeRTOS::Sleep(Pause);
	}
}

void WiFi_t::DHCPStart() {
	esp_netif_dhcp_status_t DHCPStatus;
	::esp_netif_dhcpc_get_status(WiFi_t::GetNetIf(), &DHCPStatus);

	if (DHCPStatus != ESP_NETIF_DHCP_STARTED)
		::esp_netif_dhcpc_start(WiFi_t::GetNetIf());
}

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
		esp_netif_ip_info_t ipInfo;
		ipInfo.ip.addr      = ip;
		ipInfo.gw.addr      = gw;
		ipInfo.netmask.addr = netmask;
		DHCPStop();
		::esp_netif_set_ip_info(WiFi_t::GetNetIf(), &ipInfo);
	}
	else {
		ip = 0;
		DHCPStart();
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

bool WiFi_t::IsConnectedSTA() {
	if (!m_WiFiRunning) return false;
	if (GetMode() != WIFI_MODE_STA_STR) return false;
	if (GetConnectionStatus() != ESP_OK) return false;

	return true;
}

