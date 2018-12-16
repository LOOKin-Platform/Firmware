#ifndef WIFI_HANDLER
#define WIFI_HANDLER

#include <esp_log.h>

#include <esp_ping.h>
#include <ping.h>

#include <netdb.h>
#include <mdns.h>

static FreeRTOS::Timer		*IPDidntGetTimer;
static FreeRTOS::Semaphore	IsCorrectIPData 	= FreeRTOS::Semaphore("CorrectTCPIPData");
static bool 				IsIPCheckSuccess 	= false;
static bool					IsEventDrivenStart	= false;

esp_err_t pingResults(ping_target_id_t msgType, esp_ping_found * pf) {
	ESP_LOGI("tag","ping. Received %d, Sended %d", pf->recv_count, pf->send_count);

	if (pf->recv_count > 0) {
		IsIPCheckSuccess = true;
		IsCorrectIPData.Give();
	}

	if (pf->send_count == Settings.WiFi.PingAfterConnect.Count && pf->recv_count == 0) {
		IsIPCheckSuccess = false;

		::tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
		::tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);

		IsCorrectIPData.Give();
	}

	return ESP_OK;
}

void IPDidntGetCallback(FreeRTOS::Timer *pTimer) {
	Log::Add(Log::Events::WiFi::STAUndefinedIP);
	WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
}


class MyWiFiEventHandler: public WiFiEventHandler {
	public:
		MyWiFiEventHandler() {
			IPDidntGetTimer = new FreeRTOS::Timer("IPDidntGetTimer", Settings.WiFi.IPCountdown/portTICK_PERIOD_MS, pdFALSE, NULL, IPDidntGetCallback);
		}

	private:
		esp_err_t apStart() {
			Log::Add(Log::Events::WiFi::APStart);
			WebServer.Start();
			Wireless.IsFirstWiFiStart = false;

			return ESP_OK;
		}

		esp_err_t apStop() {
			Log::Add(Log::Events::WiFi::APStop);
			Wireless.IsEventDrivenStart = false;
			return ESP_OK;
		}

		esp_err_t staConnected() {
			Log::Add(Log::Events::WiFi::STAConnected);
			IPDidntGetTimer->Start();

			return ESP_OK;
		}

		esp_err_t staDisconnected(system_event_sta_disconnected_t DisconnectedInfo) {
			Log::Add(Log::Events::WiFi::STADisconnected);
			//WebServer.Stop();

			// Повторно подключится к Wi-Fi, если подключение оборвалось
			if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE) {
				Wireless.StartInterfaces();
			}

			// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
			// (отсутствие точки доступа, неправильный пароль и т.д) подключение не удалось
			if (DisconnectedInfo.reason == WIFI_REASON_BEACON_TIMEOUT	||
				DisconnectedInfo.reason == WIFI_REASON_NO_AP_FOUND 		||
			 	DisconnectedInfo.reason == WIFI_REASON_AUTH_FAIL 		||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_FAIL 		||
				DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE		||
				DisconnectedInfo.reason == WIFI_REASON_HANDSHAKE_TIMEOUT)
				WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

			return ESP_OK;
		}

		esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
			tcpip_adapter_ip_info_t StaIPInfo = WiFi.getStaIpInfo();

			IsIPCheckSuccess = false;
			IsCorrectIPData.Take("CorrectTCPIP");

			esp_ping_set_target(PING_TARGET_IP_ADDRESS, &StaIPInfo.gw.addr, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &Settings.WiFi.PingAfterConnect.Count, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RCV_TIMEO, &Settings.WiFi.PingAfterConnect.Timeout, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_DELAY_TIME, &Settings.WiFi.PingAfterConnect.Delay, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RES_FN, (void *)pingResults, 0);
			ping_init();

			IsCorrectIPData.Wait("CorrectTCPIP");
			ping_deinit();

			IPDidntGetTimer->Stop();

			if (!IsIPCheckSuccess)
				return ESP_OK;

			BLEServer.SwitchToPublicMode();

			Wireless.IsFirstWiFiStart = false;

			Network.UpdateWiFiIPInfo(WiFi.GetStaSSID(), StaIPInfo);

			WebServer.Start();

			Network.IP = event_sta_got_ip.ip_info;

			WebServer.UDPSendBroacastFromQueue();

			if (!Wireless.IsEventDrivenStart) {
				WebServer.UDPSendBroadcastAlive();
				WebServer.UDPSendBroadcastDiscover();
			}

			Log::Add(Log::Events::WiFi::STAGotIP, Converter::IPToUint32(event_sta_got_ip.ip_info));

			Time::ServerSync(Settings.TimeSync.ServerHost, Settings.TimeSync.APIUrl);
			Wireless.IsEventDrivenStart = false;

		    esp_err_t err = mdns_init();
		    if (err) {
		        ESP_LOGE("!", "MDNS Init failed: %d", err);
		        return ESP_OK;
		    }

		    mdns_hostname_set(Device.IDToString().c_str());
		    string InstanceName = "LOOK.in " + Device.TypeToString() + " " + Device.IDToString();
		    mdns_instance_name_set(InstanceName.c_str());

			return ESP_OK;
		}
};

class WiFiUptimeHandler {
	public:
		static void Pool();
	private:
		static uint64_t WiFiStartedTime;
		static uint32_t BatteryUptime;
};

uint64_t WiFiUptimeHandler::WiFiStartedTime = 0;
uint32_t WiFiUptimeHandler::BatteryUptime 	= 0;

void WiFiUptimeHandler::Pool() {
	if (Device.PowerMode == DevicePowerMode::CONST && !WiFi.IsRunning()) {
		WiFiStartedTime = 0;
		Network.KeepWiFiTimer = 0;
		BatteryUptime = Settings.WiFi.BatteryUptime;
		Wireless.StartInterfaces();
	}

	if (Device.PowerMode == DevicePowerMode::CONST) {
		BatteryUptime = Settings.WiFi.BatteryUptime;
		return;
	}

	if (WiFi.IsRunning()) {
		if (WiFiStartedTime == 0) 		WiFiStartedTime = Time::UptimeU();

		if (Network.KeepWiFiTimer > 0) 	{
			Network.KeepWiFiTimer -= Settings.Pooling.Interval;
			if (Network.KeepWiFiTimer < Settings.Pooling.Interval)
				Network.KeepWiFiTimer = Settings.Pooling.Interval;
		}

		if (BatteryUptime > 0) {
			if (Time::UptimeU() >= WiFiStartedTime + BatteryUptime * 1000
					&& Network.KeepWiFiTimer <= 0) {
				WiFiStartedTime = 0;
				BatteryUptime = 0;
				//WebServer.Stop();
				WiFi.Stop();
			}
		}
		else {
			if (Time::UptimeU() >= WiFiStartedTime + Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second * 1000000
					&& Network.KeepWiFiTimer <= 0) {
				WiFiStartedTime = 0;
				//WebServer.Stop();
				WiFi.Stop();
			}
		}
	}
	else {
		if (Wireless.IsPeriodicPool()) {
			IsEventDrivenStart = false;
			Wireless.StartInterfaces();
		}
	}
}

#endif
