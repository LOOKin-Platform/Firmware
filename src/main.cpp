#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>
#include <esp_system.h>

#include "Globals.h"

#include "Log.h"
#include "UART.h"
#include "WiFi.h"
#include "FreeRTOSWrapper.h"
#include "DateTime.h"
#include "Memory.h"
#include "Sleep.h"

#include "handlers/Pooling.cpp"

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

static char tag[] = "Main";

void app_main(void) {

	NVS::Init();

	if (!Log::VerifyLastBoot()) {
		Log::Add(LOG_DEVICE_ROLLBACK);
		OTA::Rollback();
	}

	Log::Add(LOG_DEVICE_ON);

	Settings.eFuse.ReadData();

	Network.WiFiScannedList = WiFi.Scan();

	Time::SetTimezone();

	Device.Init();
	Network.Init();
	Automation.Init();

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands		= Command_t::GetCommandsForDevice();

	WiFi.AddDNSServer("8.8.8.8");
	WiFi.AddDNSServer("8.8.4.4");
	WiFi.SetWiFiEventHandler(new MyWiFiEventHandler());

	ESP_LOGI("tag", "Network.WiFiConnect()) start");

	Wireless.StartInterfaces();

	Log::Add(LOG_DEVICE_STARTED);

	//BLEClient.Scan();
	//BLEServer.StartAdvertising();

	static Pooling_t Pooling = Pooling_t();
	Pooling.start();

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}

