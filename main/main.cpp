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

#define ARP_TABLE_SIZE 256

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

RemoteControl_t		RemoteControl;
LocalMQTT_t			LocalMQTT;

const char tag[] = "Main";

extern "C" void app_main() 
{
	NVS::Init();
	/*
	IRLib::TestAll();
	FreeRTOS::Sleep(5000);
	*/

	//esp_log_level_set("*", ESP_LOG_VERBOSE);    // enable WARN logs from WiFi stack
	esp_log_level_set("wifi", ESP_LOG_WARN);      // set warning log level for wifi

	::esp_phy_erase_cal_data_in_nvs(); // clear PHY RF data - tried to do this to make wifi work clearear
	Settings.eFuse.ReadDataOrInit();

	Pooling_t::BARCheck();
	BootAndRestore::OnDeviceStart();

	ESP_LOGI("Current Firmware:", "%s", Settings.Firmware.ToString().c_str());

	Network.WiFiScannedList = WiFi.Scan();

	Time::SetTimezone();

	Device.Init();
	Network.Init();
	Automation.Init();

	LocalMQTT.Init();
	RemoteControl.Init();

	Data = DataEndpoint_t::GetForDevice();
	Data->Init();
	Storage.Init();

	// Remote temporary hack
	if (Settings.eFuse.Type == Settings.Devices.Remote) {
		GPIO::Setup(GPIO_NUM_22);
		GPIO::Write(GPIO_NUM_22, 0);
	}

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands		= Command_t::GetCommandsForDevice();

	HomeKit::Init();

	WiFi.SetSTAHostname(Settings.WiFi.APSSID);
	WiFi.SetWiFiEventHandler(new MyWiFiEventHandler());

	BLEServer.StartAdvertising();

	if (Network.WiFiSettings.size() == 0) // first start for HID devices
		BLEServer.ForceHIDMode(BASIC);

	Pooling_t::Pool();

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}
