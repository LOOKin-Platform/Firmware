/*
MDNS::MDNS() {
	esp_err_t errRc = ::mdns_init(TCPIP_ADAPTER_IF_STA, &m_mdns_server);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "mdns_init: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		abort();
	}
}
MDNS::~MDNS() {
	if (m_mdns_server != nullptr) {
		mdns_free(m_mdns_server);
	}
	m_mdns_server = nullptr;
}
*/


/**
 * @brief Define the service for mDNS.
 *
 * @param [in] service
 * @param [in] proto
 * @param [in] port
 * @return N/A.
 */
/*
void MDNS::serviceAdd(const std::string& service, const std::string& proto, uint16_t port) {
	serviceAdd(service.c_str(), proto.c_str(), port);
} // serviceAdd
void MDNS::serviceInstanceSet(const std::string& service, const std::string& proto, const std::string& instance) {
	serviceInstanceSet(service.c_str(), proto.c_str(), instance.c_str());
} // serviceInstanceSet
void MDNS::servicePortSet(const std::string& service, const std::string& proto, uint16_t port) {
	servicePortSet(service.c_str(), proto.c_str(), port);
} // servicePortSet
void MDNS::serviceRemove(const std::string& service, const std::string& proto) {
	serviceRemove(service.c_str(), proto.c_str());
} // serviceRemove
*/


/**
 * @brief Set the mDNS hostname.
 *
 * @param [in] hostname The host name to set against the mDNS.
 * @return N/A.
 */
/*
void MDNS::setHostname(const std::string& hostname) {
	setHostname(hostname.c_str());
} // setHostname
*/


/**
 * @brief Set the mDNS instance.
 *
 * @param [in] instance The instance name to set against the mDNS.
 * @return N/A.
 */
/*
void MDNS::setInstance(const std::string& instance) {
	setInstance(instance.c_str());
} // setInstance
*/


/**
 * @brief Define the service for mDNS.
 *
 * @param [in] service
 * @param [in] proto
 * @param [in] port
 * @return N/A.
 */
/*
void MDNS::serviceAdd(const char* service, const char* proto, uint16_t port) {
	esp_err_t errRc = ::mdns_service_add(m_mdns_server, service, proto, port);
 	if (errRc != ESP_OK) {
 		ESP_LOGE(LOG_TAG, "mdns_service_add: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
 		abort();
 	}
} // serviceAdd
void MDNS::serviceInstanceSet(const char* service, const char* proto, const char* instance) {
  esp_err_t errRc = ::mdns_service_instance_set(m_mdns_server, service, proto, instance);
  if (errRc != ESP_OK) {
  	ESP_LOGE(LOG_TAG, "mdns_service_instance_set: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
  	abort();
  }
} // serviceInstanceSet
void MDNS::servicePortSet(const char* service, const char* proto, uint16_t port) {
  esp_err_t errRc = ::mdns_service_port_set(m_mdns_server, service, proto, port);
  if (errRc != ESP_OK) {
  	ESP_LOGE(LOG_TAG, "mdns_service_port_set: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
  	abort();
  }
} // servicePortSet
void MDNS::serviceRemove(const char* service, const char* proto) {
	esp_err_t errRc = ::mdns_service_remove(m_mdns_server, service, proto);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "mdns_service_remove: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		abort();
	}
} // serviceRemove
*/


/**
 * @brief Set the mDNS hostname.
 *
 * @param [in] hostname The host name to set against the mDNS.
 * @return N/A.
 */
/*
void MDNS::setHostname(const char* hostname) {
  esp_err_t errRc = ::mdns_set_hostname(m_mdns_server,hostname);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "mdns_set_hostname: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		abort();
	}
} // setHostname
*/


/**
 * @brief Set the mDNS instance.
 *
 * @param [in] instance The instance name to set against the mDNS.
 * @return N/A.
 */
/*
void MDNS::setInstance(const char* instance) {
	esp_err_t errRc = ::mdns_set_instance(m_mdns_server, instance);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "mdns_set_instance: rc=%d %s", errRc, GeneralUtils::ErrorToString(errRc));
		abort();
	}
} // setInstance
*/
