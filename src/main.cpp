using namespace std;

#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>

#include <drivers/WiFi/WiFi.h>
#include <drivers/WiFi/WiFiEventHandler.h>
#include <drivers/OTA/OTA.h>
#include "../components/curl/include/curl/curl.h"

#include "include/Globals.h"
#include "include/WebServer.h"
#include "include/Device.h"

extern "C" {
	int app_main(void);
}

static char tag[] = "Main";

WiFi_t							*WiFi 			= new WiFi_t();
WebServer_t 				*WebServer 	= new WebServer_t();
Device_t						*Device			= new Device_t();
Network_t						*Network		= new Network_t();

vector<Sensor_t*>		Sensors 		= Sensor_t::GetSensorsForDevice();
vector<Command_t*>	Commands 		= Command_t::GetCommandsForDevice();

class MyWiFiEventHandler: public WiFiEventHandler {

	esp_err_t apStart() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): apStart");
		//WebServer->Start();
		return ESP_OK;
	}

	esp_err_t apStop() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): apStop");
		//WebServer->Stop();
		return ESP_OK;
	}

	esp_err_t staConnected() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staConnected");
		//WebServer->Start();
		return ESP_OK;
	}

//	esp_err_t staStop() {}

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

	esp_err_t staGotIp(tcpip_adapter_ip_info_t IpInfo) {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staGotIp");
		ESP_LOGI(tag, "!%s", inet_ntoa(IpInfo.ip));

		return ESP_OK;
	}

};

int app_main(void)
{
	esp_log_level_set("*", ESP_LOG_DEBUG);
	ESP_LOGD(tag, "App started");

	::nvs_flash_init();

	Device->Init();
	Network->Init();

	WiFi->setWifiEventHandler(new MyWiFiEventHandler());

	if (Network->WiFiSSID != "")
		WiFi->ConnectAP(Network->WiFiSSID, Network->WiFiPassword);
	else
		WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

	WebServer->Start();

  return 0;
}
