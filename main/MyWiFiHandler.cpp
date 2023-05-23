#include "MyWiFiHandler.h" 
#include "MatterWiFi.h"
#include "HandlersPooling.h"

static char MyWiFiTag[] = "WiFiHandler";

MyWiFiEventHandler::MyWiFiEventHandler() {
	IPDidntGetTimer = new FreeRTOS::Timer("IPDidntGetTimer", Settings.WiFi.IPCountdown/portTICK_PERIOD_MS, pdFALSE, NULL, IPDidntGetCallback);

	const esp_timer_create_args_t TimerArgs = {
		.callback 			= &RemoteControlStartTimerCallback,
		.arg 				= NULL,
		.dispatch_method 	= ESP_TIMER_TASK,
		.name				= "RCStartTimer",
		.skip_unhandled_events = false
	};

	::esp_timer_create(&TimerArgs, &RemoteControlStartTimer);
}

void MyWiFiEventHandler::Generic(void * arg, esp_event_base_t eventBase, int32_t eventId, void * eventData) {
	if (Matter::IsEnabledForDevice())
		MatterWiFi::GenericHandler(arg, eventBase, eventId, eventData);
}

esp_err_t MyWiFiEventHandler::apStart() {
	Log::Add(Log::Events::WiFi::APStart);

/*
	if (Matter::IsEnabledForDevice())
		MatterWiFi::OnAPStart();
*/
	WebServer.UDPStart();

	ConnectionTries = 0;

	Wireless.IsFirstWiFiStart = false;

	if (IsConnectedBefore)
		HandlersPooling_t::WiFiUptimeHandler::SetClientModeNextTime(Settings.WiFi.STAModeReconnect);
	else
		HandlersPooling_t::WiFiUptimeHandler::SetClientModeNextTime(Settings.WiFi.STAModeInterval);

	BootAndRestore::MarkDeviceStartedWithDelay(Settings.BootAndRestore.APSuccessDelay);

	MyBLEServer.ForceHIDMode(BASIC);

#if CONFIG_IDF_TARGET_ESP32
	PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_BALANCE);
#endif

	return ESP_OK;
}

esp_err_t MyWiFiEventHandler::apStop() {
	Log::Add(Log::Events::WiFi::APStop);

/*
	if (Matter::IsEnabledForDevice())
		MatterWiFi::OnAPStop();
*/

	//WebServer.HTTPStop();
	WebServer.UDPStop();

	Wireless.IsEventDrivenStart = false;

#if CONFIG_IDF_TARGET_ESP32
	PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_WIFI);
#endif

	return ESP_OK;
}

esp_err_t MyWiFiEventHandler::staStart() {
	Log::Add(Log::Events::WiFi::STAStarted);

/*
	if (!Matter::IsEnabledForDevice())
		MatterWiFi::OnSTAStart();
*/
	Wireless.IsFirstWiFiStart = false;

#if CONFIG_IDF_TARGET_ESP32
	PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_WIFI);
#endif

	return ESP_OK;

}

esp_err_t MyWiFiEventHandler::staConnected() {
	Log::Add(Log::Events::WiFi::STAConnected);

/*
	if (Matter::IsEnabledForDevice())
		MatterWiFi::OnSTAConnected();
*/
	ConnectionTries = 0;

#if CONFIG_IDF_TARGET_ESP32
	PowerManagement::SetWirelessPriority(ESP_COEX_PREFER_WIFI);
#endif

	if (!Matter::IsEnabledForDevice())
		IPDidntGetTimer->Start();

	return ESP_OK;
}

esp_err_t MyWiFiEventHandler::ConnectionTimeout() {
	WiFi.StartAP(Settings.WiFi.APSSID);
	return ESP_OK;
}

esp_err_t MyWiFiEventHandler::staDisconnected(wifi_event_sta_disconnected_t DisconnectedInfo) {
	Log::Add(Log::Events::WiFi::STADisconnected, (uint32_t)DisconnectedInfo.reason);

	::esp_timer_stop(RemoteControlStartTimer);

/*
	if (Matter::IsEnabledForDevice())
		MatterWiFi::OnSTADisconnected();
*/
	//WebServer.HTTPStop();
	WebServer.UDPStop();

	LocalMQTT.Stop();
	RemoteControl.Stop();

	if (Device.Status == UPDATING)
		Device.Status = RUNNING;

	if (WiFi_t::GetWiFiNetworkSwitch())
		return ESP_OK;

	ESP_LOGE("DISCONNECT REASON", "%d, Tries %d", DisconnectedInfo.reason, ConnectionTries);

	if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE ||
		DisconnectedInfo.reason == WIFI_REASON_ASSOC_EXPIRE ||
		DisconnectedInfo.reason == WIFI_REASON_BEACON_TIMEOUT)
	{
		FreeRTOS::Sleep(5000);
	}

	// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
	// (отсутствие точки доступа, неправильный пароль и т.д) подключение не удалось
	if (DisconnectedInfo.reason == WIFI_REASON_NO_AP_FOUND 				||
		DisconnectedInfo.reason == WIFI_REASON_AUTH_FAIL 				||
		DisconnectedInfo.reason == WIFI_REASON_ASSOC_FAIL				||
		DisconnectedInfo.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT	||
		ConnectionTries > 5) {
		ConnectionTries = 0;
		WiFi.StartAP(Settings.WiFi.APSSID);
	}
	else
	{ // Повторно подключится к Wi-Fi, если подключение оборвалось
		Wireless.IsFirstWiFiStart = true;
		ConnectionTries++;
		Wireless.StartInterfaces();
	}

	return ESP_OK;
}

esp_err_t MyWiFiEventHandler::staGotIPv6(ip_event_got_ip6_t GotIPv6Info) {
	/*
	if (Matter::IsEnabledForDevice())
		MatterWiFi::STAGotIPv6(GotIPv6Info);
	*/

	return ESP_OK;
}

esp_err_t MyWiFiEventHandler::staGotIPv4(ip_event_got_ip_t GotIPv4Info) {
/*

	if (Matter::IsEnabledForDevice())
		MatterWiFi::STAGotIP(GotIPv4Info);
*/
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

	//if (!HomeKit::IsEnabledForDevice())
	//	WebServer.HTTPStart();

	WebServer.UDPStart();

	Network.IP = GotIPv4Info.ip_info;

	ESP_LOGI(MyWiFiTag, "GOT IP %s", Network.IPToString().c_str());

	WebServer.UDPSendBroacastFromQueue();

	if (!Wireless.IsEventDrivenStart) {
		WebServer.UDPSendBroadcastAlive();
		WebServer.UDPSendBroadcastDiscover();
	}

	Log::Add(Log::Events::WiFi::STAGotIP, Converter::IPToUint32(GotIPv4Info.ip_info));

	Wireless.IsEventDrivenStart = false;

	esp_err_t err = ESP_OK;

	if (!Matter::IsEnabledForDevice())
	{		
		err = mdns_init();
		if (err)
			ESP_LOGE("!", "MDNS Init failed: %d", err);
	}

	if (err == ESP_OK)
	{
		err = mdns_hostname_set(Converter::ToLower(Device.IDToString()).c_str());

		if (err !=ESP_OK)
			ESP_LOGE("!", "MDNS hostname set failed: %d", err);

		string InstanceName = "LOOKin_" + Converter::ToLower(Device.TypeToString()) + "_" + Converter::ToLower(Device.IDToString());
		mdns_instance_name_set(InstanceName.c_str());

		mdns_service_add(NULL, "_lookin", "_tcp", Settings.WiFi.MDNSServicePort, NULL, 0);

		string HTTPServiceName = Settings.Bluetooth.DeviceNamePrefix + Device.IDToString();
		mdns_service_instance_name_set("_lookin", "_tcp", HTTPServiceName.c_str());

		MDNSSetServiceText();
	}
	//mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
	
	IsConnectedBefore = true;

	Time::ServerSync(Settings.ServerUrls.SyncTime);
    HTTPClient::Query(Settings.ServerUrls.Ping, QueryType::GET, true, true);

	::esp_timer_stop(RemoteControlStartTimer);
	::esp_timer_start_once(RemoteControlStartTimer, 5000000);

	BootAndRestore::MarkDeviceStartedWithDelay(Settings.BootAndRestore.STASuccessADelay);

	MyBLEServer.ForceHIDMode(HID);

	return ESP_OK;
}

void MyWiFiEventHandler::GatewayPingSuccess(esp_ping_handle_t hdl, void *args)
{
	esp_netif_ip_info_t GatewayIP = WiFi.GetIPInfo();

	if (string(inet_ntoa(GatewayIP.gw)) == string("0.0.0.0"))
		IsIPCheckSuccess = false;
	else
		IsIPCheckSuccess = true;

	IsCorrectIPData.Give();
}

void MyWiFiEventHandler::GatewayPingEnd(esp_ping_handle_t hdl, void *args)
{
    uint32_t ReceivedCount;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &ReceivedCount, sizeof(ReceivedCount));

	if (ReceivedCount == 0) {
		IsIPCheckSuccess = false;
		IsCorrectIPData.Give();

		ESP_LOGI(MyWiFiTag, "gateway does not ping. Waiting for 5s and restart dhcp");
		FreeRTOS::Sleep(5000);

		WiFi_t::DHCPStop(1000);
		WiFi_t::DHCPStart();
	}
}

void MyWiFiEventHandler::IPDidntGetCallback(FreeRTOS::Timer *pTimer) {
	Log::Add(Log::Events::WiFi::STAUndefinedIP);
	WiFi.StartAP(Settings.WiFi.APSSID);
}

void MyWiFiEventHandler::RemoteControlStartTimerCallback(void *Param) {
	LocalMQTT.Start();

	if (LocalMQTT.IsCredentialsSet() && LocalMQTT.GetIsActive())
		FreeRTOS::Sleep(2500);

	RemoteControl.Start();
}

void MyWiFiEventHandler::MDNSSetServiceText() {
	mdns_txt_item_t ServiceTxtData[6];
	ServiceTxtData[0].key = "id";
	ServiceTxtData[0].value = Device.IDToString().c_str();

	ServiceTxtData[1].key = "manufactorer";
	ServiceTxtData[1].value = "LOOKin";

	ServiceTxtData[2].key = "type";
	ServiceTxtData[2].value = Device.Type.ToHexString().c_str();

	ServiceTxtData[3].key = "isbattery";
	ServiceTxtData[3].value = Device.Type.IsBattery() ? "0" : "1";

	ServiceTxtData[4].key = "automation";
	ServiceTxtData[4].value = Converter::ToHexString(Automation.CurrentVersion(),8).c_str();

	ServiceTxtData[5].key = "storage";
	ServiceTxtData[5].value = Converter::ToHexString(Storage.CurrentVersion(),4).c_str();

	mdns_service_txt_set("_lookin", "_tcp", ServiceTxtData, 6);
}

