using namespace std;

#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "Globals.h"

#include "WiFiWrapper.h"
#include "WiFiEventHandler.h"
#include "TimerWrapper.h"
#include "SystemTime.h"

static char tag[] = "Main";

extern "C" {
	void app_main(void);
}

WiFi_t							*WiFi 			= new WiFi_t();
WebServer_t 				*WebServer 	= new WebServer_t();

Device_t						*Device			= new Device_t();
Network_t						*Network		= new Network_t();
Automation_t				*Automation	= new Automation_t();

vector<Sensor_t*>		Sensors 		= Sensor_t::GetSensorsForDevice();
vector<Command_t*>	Commands 		= Command_t::GetCommandsForDevice();

Timer_t *IPDidntGetTimer;
static void IPDidntGetCallback(Timer_t *pTimer) { WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD); }

class MyWiFiEventHandler: public WiFiEventHandler {

	esp_err_t apStart() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): apStart");
		return ESP_OK;
	}

	esp_err_t apStop() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): apStop");
		return ESP_OK;
	}

	esp_err_t staConnected() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staConnected");
		IPDidntGetTimer->Start();

		return ESP_OK;
	}

	esp_err_t staDisconnected(system_event_sta_disconnected_t DisconnectedInfo) {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staDisconnected");
		//WebServer->Stop();

		// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
		// (отсутсвие точки доступа, неправильный пароль и т.д) подключение не удалось
		if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE		||
				DisconnectedInfo.reason == WIFI_REASON_BEACON_TIMEOUT	||
				DisconnectedInfo.reason == WIFI_REASON_NO_AP_FOUND 		||
			 	DisconnectedInfo.reason == WIFI_REASON_AUTH_FAIL 			||
				DisconnectedInfo.reason == WIFI_REASON_ASSOC_FAIL 		||
				DisconnectedInfo.reason == WIFI_REASON_HANDSHAKE_TIMEOUT)
			WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

			return ESP_OK;
	}

	esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
		IPDidntGetTimer->Stop();

		Network->IP = event_sta_got_ip.ip_info;
		WebServer->UDPSendBroadcastAlive();
		WebServer->UDPSendBroadcastDiscover();

		Time::ServerSync(TIME_SERVER_HOST, TIME_API_URL);

		return ESP_OK;
	}
};

void app_main(void) {

	esp_log_level_set("*", ESP_LOG_DEBUG);
	ESP_LOGD(tag, "App started");

	::nvs_flash_init();

	Time::SetTimezone();

	Device->Init();
	Network->Init();
	Automation->Init();

	WiFi->setWifiEventHandler(new MyWiFiEventHandler());
	IPDidntGetTimer =	new Timer_t("IPDidntGetTimer",WIFI_IP_COUNTDOWN/portTICK_PERIOD_MS, pdFALSE, NULL, IPDidntGetCallback);

	if (Network->WiFiSSID != "")
		WiFi->ConnectAP(Network->WiFiSSID, Network->WiFiPassword);
	else
		WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

	WebServer->Start();
}
