#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>
#include <esp_pm.h>
#include <esp_system.h>
#include <esp_phy_init.h>

#include "Globals.h"

#include "Log.h"
#include "UART.h"
#include "WiFi.h"
#include "FreeRTOSWrapper.h"
#include "DateTime.h"
#include "Memory.h"
#include "Sleep.h"

#include "handlers/Pooling.cpp"
//#include "HomeKit/hap.h"

using namespace std;

extern "C" {
	void app_main(void);
}

Settings_t 			Settings;

WiFi_t				WiFi;
WebServer_t 		WebServer;

#if defined(CONFIG_BT_ENABLED)
BLEServer_t			BLEServer;
BLEClient_t			BLEClient;
#endif /* Bluetooth enabled */

Device_t			Device;
Network_t			Network;
Automation_t		Automation;
Storage_t			Storage;

Wireless_t			Wireless;

vector<Sensor_t*>	Sensors;
vector<Command_t*>	Commands;

MQTT_t				MQTT;

const char tag[] = "Main";

void app_main(void) {
	NVS::Init();

	::esp_phy_erase_cal_data_in_nvs(); // clear PHY RF data - tried to do this to make wifi work clearear

	if (!Log::VerifyLastBoot()) {
		Log::Add(Log::Events::System::DeviceRollback);
		OTA::Rollback();
	}

	Settings.eFuse.ReadData();
	Log::Add(Log::Events::System::DeviceOn);

	#if defined(CONFIG_PM_ENABLE)

    esp_pm_config_esp32_t pm_config = {
		.max_cpu_freq = RTC_CPU_FREQ_160M,
		.min_cpu_freq = RTC_CPU_FREQ_XTAL
    };

    esp_err_t ret;
    if((ret = esp_pm_configure(&pm_config)) != ESP_OK)
        ESP_LOGI(tag, "pm config error %s\n",  ret == ESP_ERR_INVALID_ARG ?  "ESP_ERR_INVALID_ARG": "ESP_ERR_NOT_SUPPORTED");

	#endif

	Network.WiFiScannedList = WiFi.Scan();

	Time::SetTimezone();

	Device.Init();
	Network.Init();
	Automation.Init();
	MQTT.Init();

	// Remote temporary hack
	GPIO::Setup(GPIO_NUM_22);
	GPIO::Write(GPIO_NUM_22, 0);

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands		= Command_t::GetCommandsForDevice();

	WiFi.SetSTAHostname(WIFI_AP_NAME);
	WiFi.AddDNSServer("8.8.8.8");
	WiFi.AddDNSServer("8.8.4.4");
	WiFi.SetWiFiEventHandler(new MyWiFiEventHandler());

	//Wireless.StartInterfaces();

	Log::Add(Log::Events::System::DeviceStarted);

	BLEServer.StartAdvertising("", true);
	//BLEClient.Scan(60);

	static Pooling_t Pooling = Pooling_t();
	Pooling.Start();

	while (1) {
		FreeRTOS::Sleep(1000);
	}

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}
