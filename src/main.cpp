using namespace std;

#include "string.h"
#include "stdint.h"
#include "stdio.h"

extern "C" {
	#include "esp_wifi.h"
	#include "esp_system.h"
	#include "esp_event.h"
	#include "esp_event_loop.h"

	#include "freertos/event_groups.h"

	#include "nvs_flash.h"
	#include "esp_log.h"

	int app_main(void);
}

#include "../components/curl/include/curl/curl.h"

#include "drivers/OTA/OTA.h"

#include "include/Globals.h"
#include "include/WebServer.h"
#include "include/Device.h"

static EventGroupHandle_t wifi_event_group;
WebServer_t 			WebServer;

Device_t					Device;
vector<Sensor_t> 	Sensors 	= Sensor_t::GetSensorsForDevice();
vector<Command_t>	Commands 	= Command_t::GetCommandsForDevice();

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        //xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

int app_main(void)
{
    nvs_flash_init();

    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

		string ssid = "NAT";
		string password = "NAT123";

		wifi_config_t apConfig;
		memset(&apConfig, 0, sizeof(apConfig));
		memcpy(apConfig.ap.ssid, ssid.data(), ssid.size());

		apConfig.ap.ssid_len = 0;
		memcpy(apConfig.ap.password, password.data(), password.size());
		apConfig.ap.channel = 0;
		apConfig.ap.authmode = WIFI_AUTH_OPEN;
		apConfig.ap.ssid_hidden = 0;
		apConfig.ap.max_connection = 4;
		apConfig.ap.beacon_interval = 100;

		ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_AP, &apConfig) );
		ESP_ERROR_CHECK( esp_wifi_start() );

		WebServer.Start();

    return 0;
}
