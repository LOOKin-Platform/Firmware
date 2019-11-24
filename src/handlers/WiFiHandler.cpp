#ifndef WIFI_HANDLER
#define WIFI_HANDLER

#include <esp_log.h>
#include <DateTime.h>
#include <esp_ping.h>
#include <ping/ping.h>

#include <netdb.h>
#include <mdns.h>

static char HandlerTag[] = "WiFiHandler";

static FreeRTOS::Timer		*IPDidntGetTimer;
static FreeRTOS::Semaphore	IsCorrectIPData 	= FreeRTOS::Semaphore("CorrectTCPIPData");
static bool 				IsIPCheckSuccess 	= false;
static bool					IsEventDrivenStart	= false;
static bool					IsConnectedBefore 	= false;

class WiFiUptimeHandler {
	public:
		static void Start();
		static void Pool();
		static void SetClientModeNextTime(uint32_t);

		static bool GetIsIPCheckSuccess();
	private:
		static uint64_t WiFiStartedTime;
		static uint32_t BatteryUptime;
		static uint32_t ClientModeNextTime;
};

uint64_t WiFiUptimeHandler::WiFiStartedTime 	= 0;
uint32_t WiFiUptimeHandler::BatteryUptime 		= 0;
uint32_t WiFiUptimeHandler::ClientModeNextTime	= 0;

void WiFiUptimeHandler::SetClientModeNextTime(uint32_t Value) {
	ClientModeNextTime = Time::Unixtime() + Value;
}

void WiFiUptimeHandler::Start() {
	if (Device.PowerMode == DevicePowerMode::CONST && !WiFi.IsRunning()) {
		WiFiStartedTime = 0;
		Network.KeepWiFiTimer = 0;
		BatteryUptime = Settings.WiFi.BatteryUptime;
		Wireless.StartInterfaces();
		return;
	}
}

void IRAM_ATTR WiFiUptimeHandler::Pool() {
	if (Device.PowerMode == DevicePowerMode::CONST ||
	   (Device.PowerMode == DevicePowerMode::BATTERY && !Device.SensorMode)) {
		BatteryUptime = Settings.WiFi.BatteryUptime;

		if (Time::Unixtime() > ClientModeNextTime && WiFi_t::GetMode() == WIFI_MODE_AP_STR && ClientModeNextTime > 0)
		{
			IsConnectedBefore = false;
			ClientModeNextTime = 0;

			if (WiFi_t::GetAPClientsCount() == 0) {
				Wireless.StopWiFi();
				Wireless.StartInterfaces();
			}
			else
				SetClientModeNextTime(Settings.WiFi.STAModeReconnect);
		}

		return;
	}

	if (WiFi.IsRunning()) {
		if (WiFiStartedTime == 0)
			WiFiStartedTime = Time::UptimeU();

		if (Network.KeepWiFiTimer > 0) 	{
			Network.KeepWiFiTimer -= Settings.Pooling.Interval;
			if (Network.KeepWiFiTimer < Settings.Pooling.Interval)
				Network.KeepWiFiTimer = Settings.Pooling.Interval;
		}

		if (BatteryUptime > 0) {
			if (Time::UptimeU() >= WiFiStartedTime + BatteryUptime * 1000 && Network.KeepWiFiTimer <= 0) {
				WiFiStartedTime = 0;
				BatteryUptime = 0;
				WiFi.Stop();
			}
		}
		else {
			if (Time::UptimeU() >= WiFiStartedTime + Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second * 1000000
					&& Network.KeepWiFiTimer <= 0) {
				ESP_LOGI("WiFiHandler", "WiFi uptime for battery device expired. Stopping wifi");
				WiFiStartedTime = 0;
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

esp_err_t IRAM_ATTR pingResults(ping_target_id_t msgType, esp_ping_found * pf) {
	if (pf->recv_count > 0) {
		IsIPCheckSuccess = true;
		IsCorrectIPData.Give();
	}

	tcpip_adapter_ip_info_t GatewayIP = WiFi.getStaIpInfo();

	if (string(inet_ntoa(GatewayIP.gw)) == string("0.0.0.0") && pf->send_count > 0)
		return ESP_FAIL;

	ESP_LOGI(HandlerTag, "ping to %s. Received %d, Sended %d", inet_ntoa(GatewayIP.gw) ,pf->recv_count, pf->send_count);

	if (pf->send_count == Settings.WiFi.PingAfterConnect.Count && pf->recv_count == 0) {
		IsIPCheckSuccess = false;
		IsCorrectIPData.Give();

		ESP_LOGI(HandlerTag, "gateway does not ping. Waiting for 5s and restart dhcp");
		FreeRTOS::Sleep(5000);

		::tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);

		FreeRTOS::Sleep(1000);

		::tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
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

			if (IsConnectedBefore)
				WiFiUptimeHandler::SetClientModeNextTime(Settings.WiFi.STAModeReconnect);
			else
				WiFiUptimeHandler::SetClientModeNextTime(Settings.WiFi.STAModeInterval);

			return ESP_OK;
		}

		esp_err_t apStop() {
			Log::Add(Log::Events::WiFi::APStop);

			WebServer.Stop();

			Wireless.IsEventDrivenStart = false;
			return ESP_OK;
		}

		esp_err_t staStart() {
			Log::Add(Log::Events::WiFi::STAStarted);
			Wireless.IsFirstWiFiStart = false;
			return ESP_OK;
		}

		esp_err_t staConnected() {
			Log::Add(Log::Events::WiFi::STAConnected);
			IPDidntGetTimer->Start();

			return ESP_OK;
		}

		esp_err_t ConnectionTimeout() {
			WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
			return ESP_OK;
		}

		esp_err_t staDisconnected(system_event_sta_disconnected_t DisconnectedInfo) {
			Log::Add(Log::Events::WiFi::STADisconnected, (uint32_t)DisconnectedInfo.reason);

			WebServer.Stop();
			MQTT.Stop();

			if (Device.Status == UPDATING)
				Device.Status = RUNNING;

			if (WiFi_t::GetWiFiNetworkSwitch())
				return ESP_OK;


			if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE ||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_EXPIRE)
			{
				::esp_wifi_set_ps(WIFI_PS_NONE);
				FreeRTOS::Sleep(5000);
			}

			// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
			// (отсутствие точки доступа, неправильный пароль и т.д) подключение не удалось
			if (DisconnectedInfo.reason == WIFI_REASON_NO_AP_FOUND 		||
			 	DisconnectedInfo.reason == WIFI_REASON_AUTH_FAIL 		||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_FAIL		||
				DisconnectedInfo.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) {
				WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
			}
			else { // Повторно подключится к Wi-Fi, если подключение оборвалось
				Wireless.IsFirstWiFiStart = true;
				Wireless.StartInterfaces();
			}

			return ESP_OK;
		}

		esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
			tcpip_adapter_ip_info_t StaIPInfo = WiFi.getStaIpInfo();

			IsIPCheckSuccess = false;
			WiFi.IsIPCheckSuccess = false;

			IsCorrectIPData.Take("CorrectTCPIP");

			esp_ping_set_target(PING_TARGET_IP_ADDRESS		, &StaIPInfo.gw.addr, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &Settings.WiFi.PingAfterConnect.Count, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RCV_TIMEO		, &Settings.WiFi.PingAfterConnect.Timeout, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_DELAY_TIME		, &Settings.WiFi.PingAfterConnect.Delay, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RES_FN			, (void *)pingResults, 0);
			ping_init();

			IsCorrectIPData.Wait("CorrectTCPIP");
			ping_deinit();

			IPDidntGetTimer->Stop();

			if (!IsIPCheckSuccess)
				return ESP_OK;

			WiFi.IsIPCheckSuccess 		= true;
			Wireless.IsFirstWiFiStart 	= false;

			Network.UpdateWiFiIPInfo(WiFi.GetStaSSID(), StaIPInfo);

			WebServer.Start();

			Network.IP = event_sta_got_ip.ip_info;

			WebServer.UDPSendBroacastFromQueue();

			if (!Wireless.IsEventDrivenStart) {
				WebServer.UDPSendBroadcastAlive();
				WebServer.UDPSendBroadcastDiscover();
			}

			Log::Add(Log::Events::WiFi::STAGotIP, Converter::IPToUint32(event_sta_got_ip.ip_info));

			Wireless.IsEventDrivenStart = false;

		    esp_err_t err = mdns_init();
		    if (err) {
		        ESP_LOGE("!", "MDNS Init failed: %d", err);
		        return ESP_OK;
		    }

		    mdns_hostname_set(Device.IDToString().c_str());
		    string InstanceName = "LOOK.in " + Device.TypeToString() + " " + Device.IDToString();
		    mdns_instance_name_set(InstanceName.c_str());

			BLEServer.SwitchToPublicMode();
			IsConnectedBefore = true;

			Time::ServerSync(Settings.TimeSync.APIUrl);

			MQTT.Start();

			return ESP_OK;
		}
};

#endif
