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

#include "handlers/OverheatHandler.cpp"
#include "handlers/WiFiHandler.cpp"
#include "handlers/BluetoothHandler.cpp"

#include <esp_heap_trace.h>

using namespace std;

extern "C" {
	void app_main(void);
}

static heap_trace_record_t trace_record[1024]; // This buffer must be in internal RAM

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

	esp_err_t errRc = ::heap_trace_init_standalone(trace_record, 1024);
	if (errRc != ESP_OK) {
		ESP_LOGE(tag, "heap_trace_init_standalone error: rc=%d", errRc);
		abort();
	}

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

	//FreeRTOS::Sleep(10000);
	//WiFi.Stop();
	//FreeRTOS::Sleep(5000);
	//ESP_LOGI("tag", "event occured");
	//Wireless.SendBroadcastUpdated(0x80,"bugaga");

	//BLEClient.Scan();
	//BLEServer.StartAdvertising();

	while (1) {
		if (Time::Uptime() % 10 == 0)
			ESP_LOGI("main","RAM left %d", esp_get_free_heap_size());

		//heap_trace_start(HEAP_TRACE_LEAKS);

		/*
		ESP_LOGI(tag, "Advertising started")
		BLEServer.StartAdvertising();
		FreeRTOS::Sleep(5000);
		ESP_LOGI(tag, "Advertising stopped")
		BLEServer.StopAdvertising();
		FreeRTOS::Sleep(5000);
*/
		//}

		//if (i==4)
		//	BLEServer.StartAdvertising();

		//heap_trace_stop();
		//heap_trace_dump();

		//i++;

		OverheatHandler::Pool();
		WiFiUptimeHandler::Pool();
		BluetoothPeriodicHandler::Pool();

		FreeRTOS::Sleep(Settings.Pooling.Interval);
	}

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}

