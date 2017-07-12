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

WiFi_t						*WiFi 			= new WiFi_t();
WebServer_t 			*WebServer 	= new WebServer_t();
Device_t					*Device			= new Device_t();

vector<Sensor_t> 	Sensors 		= Sensor_t::GetSensorsForDevice();
vector<Command_t>	Commands 		= Command_t::GetCommandsForDevice();

class MyWiFiEventHandler: public WiFiEventHandler {

	esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staGotIp");
		return ESP_OK;
	}

	esp_err_t apStart() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): apStart");
		WebServer->Start();
		return ESP_OK;
	}

	esp_err_t staConnected() {
		ESP_LOGD(tag, "MyWiFiEventHandler(Class): staConnected");
		WebServer->Start();
		return ESP_OK;
	}
};

int app_main(void)
{
		esp_log_level_set("*", ESP_LOG_DEBUG);
		ESP_LOGD(tag, "App started");

		nvs_flash_init();

		Device->Init();

		MyWiFiEventHandler *eventHandler = new MyWiFiEventHandler();
		WiFi->setWifiEventHandler(eventHandler);
		WiFi->startAP("NAT", "NAT123456");

    return 0;
}
