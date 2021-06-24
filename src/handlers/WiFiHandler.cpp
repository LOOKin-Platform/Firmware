#ifndef WIFI_HANDLER
#define WIFI_HANDLER

#include <esp_log.h>
#include <DateTime.h>
#include <esp_ping.h>
#include "ping/ping_sock.h"

#include <netdb.h>
#include <mdns.h>

#include "HomeKit.h"

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

			if (WiFi_t::GetAPClientsCount() == 0 && Network.WiFiSettings.size() > 0) {
				//WiFi.DeInit();
				::esp_wifi_stop();
				//FreeRTOS::Sleep(1000);

				Network.WiFiScannedList = WiFi.Scan();
				Wireless.StartInterfaces();
				SetClientModeNextTime(Settings.WiFi.STAModeInterval);
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

static void GatewayPingSuccess(esp_ping_handle_t hdl, void *args)
{
	esp_netif_ip_info_t GatewayIP = WiFi.GetIPInfo();

	if (string(inet_ntoa(GatewayIP.gw)) == string("0.0.0.0"))
		IsIPCheckSuccess = false;
	else
		IsIPCheckSuccess = true;

	IsCorrectIPData.Give();
}

static void GatewayPingEnd(esp_ping_handle_t hdl, void *args)
{
    uint32_t ReceivedCount;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &ReceivedCount, sizeof(ReceivedCount));

	if (ReceivedCount == 0) {
		IsIPCheckSuccess = false;
		IsCorrectIPData.Give();

		ESP_LOGI(HandlerTag, "gateway does not ping. Waiting for 5s and restart dhcp");
		FreeRTOS::Sleep(5000);

		WiFi_t::DHCPStop(1000);
		WiFi_t::DHCPStart();
	}
}

void IPDidntGetCallback(FreeRTOS::Timer *pTimer) {
	Log::Add(Log::Events::WiFi::STAUndefinedIP);
	WiFi.StartAP(Settings.WiFi.APSSID);
}

class MyWiFiEventHandler: public WiFiEventHandler {
	public:
		MyWiFiEventHandler() {
			IPDidntGetTimer = new FreeRTOS::Timer("IPDidntGetTimer", Settings.WiFi.IPCountdown/portTICK_PERIOD_MS, pdFALSE, NULL, IPDidntGetCallback);
		}

	private:
		esp_err_t apStart() {
			Log::Add(Log::Events::WiFi::APStart);
			WebServer.HTTPStart();
			WebServer.UDPStart();

			Wireless.IsFirstWiFiStart = false;

			if (IsConnectedBefore)
				WiFiUptimeHandler::SetClientModeNextTime(Settings.WiFi.STAModeReconnect);
			else
				WiFiUptimeHandler::SetClientModeNextTime(Settings.WiFi.STAModeInterval);

			BootAndRestore::MarkDeviceStartedWithDelay(Settings.BootAndRestore.APSuccessDelay);

			return ESP_OK;
		}

		esp_err_t apStop() {
			Log::Add(Log::Events::WiFi::APStop);

			WebServer.HTTPStop();
			WebServer.UDPStop();

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

			if (!HomeKit::IsEnabledForDevice())
				IPDidntGetTimer->Start();

			return ESP_OK;
		}

		esp_err_t ConnectionTimeout() {
			WiFi.StartAP(Settings.WiFi.APSSID);
			return ESP_OK;
		}

		esp_err_t staDisconnected(system_event_sta_disconnected_t DisconnectedInfo) {
			Log::Add(Log::Events::WiFi::STADisconnected, (uint32_t)DisconnectedInfo.reason);

			WebServer.HTTPStop();
			WebServer.UDPStop();

			LocalMQTT.Stop();
			RemoteControl.Stop();

			if (Device.Status == UPDATING)
				Device.Status = RUNNING;

			if (WiFi_t::GetWiFiNetworkSwitch())
				return ESP_OK;


			if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE ||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_EXPIRE)
			{
				FreeRTOS::Sleep(5000);
			}

			// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
			// (отсутствие точки доступа, неправильный пароль и т.д) подключение не удалось
			if (DisconnectedInfo.reason == WIFI_REASON_NO_AP_FOUND 		||
			 	DisconnectedInfo.reason == WIFI_REASON_AUTH_FAIL 		||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_FAIL		||
				DisconnectedInfo.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) {
				WiFi.StartAP(Settings.WiFi.APSSID);
			}
			else { // Повторно подключится к Wi-Fi, если подключение оборвалось
				Wireless.IsFirstWiFiStart = true;
				Wireless.StartInterfaces();
			}

			return ESP_OK;
		}

		esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
			if (!HomeKit::IsEnabledForDevice())
			{
				esp_netif_ip_info_t StaIPInfo = WiFi.GetIPInfo();

				IsIPCheckSuccess = false;
				WiFi.IsIPCheckSuccess = false;

				IsCorrectIPData.Take("CorrectTCPIP");

				ip_addr_t ServerIP;
				ServerIP.u_addr.ip4.addr = StaIPInfo.gw.addr;
				ServerIP.type = IPADDR_TYPE_V4;

				esp_ping_config_t ping_config 	= ESP_PING_DEFAULT_CONFIG();
				ping_config.target_addr 		= ServerIP;          // target IP address
				ping_config.count 				= Settings.WiFi.PingAfterConnect.Count;    // ping in infinite mode, esp_ping_stop can stop it
				ping_config.timeout_ms			= Settings.WiFi.PingAfterConnect.Timeout;
				ping_config.interval_ms			= Settings.WiFi.PingAfterConnect.Delay;

				esp_ping_callbacks_t cbs;
				cbs.on_ping_success = GatewayPingSuccess;
				cbs.on_ping_timeout = NULL;
				cbs.on_ping_end 	= GatewayPingEnd;
				cbs.cb_args 		= NULL;  // arguments that will feed to all callback functions, can be NULL

				esp_ping_handle_t PingHandler;
				esp_ping_new_session(&ping_config, &cbs, &PingHandler);
				esp_ping_start(PingHandler);

				IsCorrectIPData.Wait("CorrectTCPIP");

				esp_ping_stop(PingHandler);
				esp_ping_delete_session(PingHandler);

				IPDidntGetTimer->Stop();

				if (!IsIPCheckSuccess)
					return ESP_OK;

				PowerManagement::SetWiFiOptions();

				WiFi.IsIPCheckSuccess 		= true;
				Wireless.IsFirstWiFiStart 	= false;

				//WiFi.ClearDNSServers();
				//WiFi.AddDNSServer("8.8.8.8");
				//WiFi.AddDNSServer("8.8.4.4");

				Network.UpdateWiFiIPInfo(WiFi.GetStaSSID(), StaIPInfo);
			}
			else
				WiFi.IsIPCheckSuccess = true;

			if (!HomeKit::IsEnabledForDevice())
				WebServer.HTTPStart();

			WebServer.UDPStart();

			Network.IP = event_sta_got_ip.ip_info;

			WebServer.UDPSendBroacastFromQueue();

			if (!Wireless.IsEventDrivenStart) {
				WebServer.UDPSendBroadcastAlive();
				WebServer.UDPSendBroadcastDiscover();
			}

			Log::Add(Log::Events::WiFi::STAGotIP, Converter::IPToUint32(event_sta_got_ip.ip_info));

			Wireless.IsEventDrivenStart = false;

			if (!HomeKit::IsEnabledForDevice())
			{
			    esp_err_t err = mdns_init();
			    if (err) {
			        ESP_LOGE("!", "MDNS Init failed: %d", err);
			        return ESP_OK;
			    }

			    err = mdns_hostname_set(Converter::ToLower(Device.IDToString()).c_str());

			    if (err !=ESP_OK)
			    	ESP_LOGE("!", "MDNS hostname set failed: %d", err);

			    string InstanceName = "LOOKin_" + Converter::ToLower(Device.TypeToString()) + "_" + Converter::ToLower(Device.IDToString());
			    mdns_instance_name_set(InstanceName.c_str());
			}
			else
			    mdns_hostname_set(Converter::ToLower(Device.IDToString()).c_str());


		    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
		    mdns_service_add(NULL, "_lookin", "_udp", Settings.WiFi.MDNSServicePort, NULL, 0);

		    string HTTPServiceName = Settings.Bluetooth.DeviceNamePrefix + Device.IDToString();
		    mdns_service_instance_name_set("_http", "_tcp", HTTPServiceName.c_str());
		    mdns_service_instance_name_set("_lookin", "_udp", HTTPServiceName.c_str());

		    mdns_txt_item_t ServiceTxtData[2];
		    ServiceTxtData[0].key = "id";
		    ServiceTxtData[0].value = Device.IDToString().c_str();
		    ServiceTxtData[1].key = "manufactorer";
		    ServiceTxtData[1].value = "LOOKin";

		    mdns_service_txt_set("_lookin", "_udp", ServiceTxtData, 2);

			IsConnectedBefore = true;

			Time::ServerSync(Settings.ServerUrls.SyncTime);

			LocalMQTT.Start();
			RemoteControl.Start();

			BootAndRestore::MarkDeviceStartedWithDelay(Settings.BootAndRestore.STASuccessADelay);

			return ESP_OK;
		}
};

#endif
