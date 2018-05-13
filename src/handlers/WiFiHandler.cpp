FreeRTOS::Timer *IPDidntGetTimer;
static void IPDidntGetCallback(FreeRTOS::Timer *pTimer) {
	Log::Add(LOG_WIFI_STA_UNDEFINED_IP);
	WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
}

class MyWiFiEventHandler: public WiFiEventHandler {
	public:
		MyWiFiEventHandler() {
			IPDidntGetTimer = new FreeRTOS::Timer("IPDidntGetTimer", Settings.WiFi.IPCountdown/portTICK_PERIOD_MS, pdFALSE, NULL, IPDidntGetCallback);
		}

	private:
		esp_err_t apStart() {
			Log::Add(LOG_WIFI_AP_START);
			return ESP_OK;
		}

		esp_err_t apStop() {
			Log::Add(LOG_WIFI_AP_STOP);
			return ESP_OK;
		}

		esp_err_t staConnected() {
			Log::Add(LOG_WIFI_STA_CONNECTED);
			IPDidntGetTimer->Start();

			return ESP_OK;
		}

		esp_err_t staDisconnected(system_event_sta_disconnected_t DisconnectedInfo) {
			Log::Add(LOG_WIFI_STA_DISCONNECTED);
			//WebServer.Stop();

			// Повторно подключится к Wi-Fi, если подключение оборвалось
			if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE)
				Network.WiFiConnect();

			// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
			// (отсутсвие точки доступа, неправильный пароль и т.д) подключение не удалось
			if (DisconnectedInfo.reason == WIFI_REASON_BEACON_TIMEOUT	||
				DisconnectedInfo.reason == WIFI_REASON_NO_AP_FOUND 		||
			 	DisconnectedInfo.reason == WIFI_REASON_AUTH_FAIL 			||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_FAIL 		||
				DisconnectedInfo.reason == WIFI_REASON_HANDSHAKE_TIMEOUT)
				WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

			return ESP_OK;
		}

		esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
			IPDidntGetTimer->Stop();

			Network.IP = event_sta_got_ip.ip_info;

			WebServer.UDPSendBroadcastAlive();
			WebServer.UDPSendBroadcastDiscover();

			Log::Add(LOG_WIFI_STA_GOT_IP, Converter::IPToUint32(event_sta_got_ip.ip_info));

			Time::ServerSync(Settings.TimeSync.ServerHost, Settings.TimeSync.APIUrl);

			return ESP_OK;
		}
};
