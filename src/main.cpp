#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>
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

#include "BootAndRestore.h"

#include "PowerManagement.h"

#include "handlers/Pooling.cpp"

using namespace std;

extern "C" {
	void app_main(void);
}

Settings_t 			Settings;

WiFi_t				WiFi;
WebServer_t 		WebServer;

BLEServer_t			BLEServer;
BLEClient_t			BLEClient;

Device_t			Device;
Network_t			Network;
Automation_t		Automation;
Storage_t			Storage;
DataEndpoint_t		*Data;

Wireless_t			Wireless;

vector<Sensor_t*>	Sensors;
vector<Command_t*>	Commands;

MQTT_t				MQTT;

const char tag[] = "Main";


void app_main(void) {
	NVS::Init();

	::esp_phy_erase_cal_data_in_nvs(); // clear PHY RF data - tried to do this to make wifi work clearear

	Settings.eFuse.ReadData();

	ESP_LOGE("Device Generation", "%d", Settings.DeviceGeneration);

	BootAndRestore::OnDeviceStart();

	PowerManagement::SetIsActive((Settings.eFuse.Type == 0x81) ? false : true);
	//PowerManagement::SetIsActive(true);

	Network.WiFiScannedList = WiFi.Scan();

	Time::SetTimezone();

	Device.Init();
	Network.Init();
	Automation.Init();
	MQTT.Init();

	Data = DataEndpoint_t::GetForDevice();
	Data->Init();

	HomeKit::Init();

	// Remote temporary hack
	if (Settings.eFuse.Type == Settings.Devices.Remote) {
		GPIO::Setup(GPIO_NUM_22);
		GPIO::Write(GPIO_NUM_22, 0);
	}

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands		= Command_t::GetCommandsForDevice();

	WiFi.SetSTAHostname(Settings.WiFi.APSSID);
	WiFi.SetWiFiEventHandler(new MyWiFiEventHandler());

	BLEServer.StartAdvertising("", true);
	//BLEClient.Scan(60);

	Pooling_t::Pool();

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}
