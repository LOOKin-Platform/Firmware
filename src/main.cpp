using namespace std;

#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <esp_system.h>

#include "Globals.h"

#include "Log.h"
#include "Convert.h"
#include "WiFi.h"
#include "WiFiEventHandler.h"
#include "FreeRTOSWrapper.h"
#include "DateTime.h"

extern "C" {
	uint8_t temprature_sens_read(void);
	void app_main(void);
}

WiFi_t							*WiFi 			= new WiFi_t();
WebServer_t 				*WebServer 	= new WebServer_t();

Device_t						*Device			= new Device_t();
Network_t						*Network		= new Network_t();
Automation_t				*Automation	= new Automation_t();

vector<Sensor_t*>		Sensors;
vector<Command_t*>	Commands;

FreeRTOS::Timer *IPDidntGetTimer;
static void IPDidntGetCallback(FreeRTOS::Timer *pTimer) {
	Log::Add(LOG_WIFI_STA_UNDEFINED_IP);
	WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
}

class MyWiFiEventHandler: public WiFiEventHandler {
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
		//WebServer->Stop();

		// Повторно подключится к Wi-Fi, если подключение оборвалось
		if (DisconnectedInfo.reason == WIFI_REASON_AUTH_EXPIRE)
			WiFi->ConnectAP(Network->WiFiSSID, Network->WiFiPassword);

		// Перезапустить Wi-Fi в режиме точки доступа, если по одной из причин
		// (отсутсвие точки доступа, неправильный пароль и т.д) подключение не удалось
		if (DisconnectedInfo.reason == WIFI_REASON_BEACON_TIMEOUT	||
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

		Log::Add(LOG_WIFI_STA_GOT_IP, Converter::IPToUint32(event_sta_got_ip.ip_info));

		Time::ServerSync(TIME_SERVER_HOST, TIME_API_URL);

		return ESP_OK;
	}
};

void OverheatControl(void *pvParameters) {
	bool IsOverheated = false;

	while (1) {
		uint8_t ChipTemperature = temprature_sens_read();
		ChipTemperature = (uint8_t)floor((ChipTemperature - 32) * (5.0/9.0) + 0.5); // From Fahrenheit to Celsius
		Device->Temperature = ChipTemperature;

		if (ChipTemperature > 90) {
			IsOverheated = true;
			Log::Add(LOG_DEVICE_OVERHEAT);
			for (auto& Command : Commands)
				Command->Overheated();
		}

		if (ChipTemperature < 77) {
			if (IsOverheated) {
				IsOverheated = false;
				Log::Add(LOG_DEVICE_COOLLED);
			}
		}

    FreeRTOS::Sleep(OVERHEET_POOLING);
  }
}

void app_main(void) {
	esp_log_level_set("*", ESP_LOG_DEBUG);
	::nvs_flash_init();

	if (!Log::VerifyLastBoot()) {
		Log::Add(LOG_DEVICE_ROLLBACK);
		OTA::Rollback();
	}

	Log::Add(LOG_DEVICE_ON);

	Time::SetTimezone();

	Device->Init();
	Network->Init();
	Automation->Init();

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands 		= Command_t::GetCommandsForDevice();

	WiFi->setWifiEventHandler(new MyWiFiEventHandler());
	IPDidntGetTimer =	new FreeRTOS::Timer("IPDidntGetTimer",WIFI_IP_COUNTDOWN/portTICK_PERIOD_MS, pdFALSE, NULL, IPDidntGetCallback);

	if (Network->WiFiSSID != "")
		WiFi->ConnectAP(Network->WiFiSSID, Network->WiFiPassword);
	else
		WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

	WebServer->Start();

	FreeRTOS::StartTaskPinnedToCore(&OverheatControl, "OverheatControl", NULL, 2048);

	Log::Add(LOG_DEVICE_STARTED);
}
