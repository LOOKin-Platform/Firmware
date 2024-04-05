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
#include "HandlersPooling.h"
#include "nimble/nimble_port_freertos.h"
#include "MyWiFiHandler.h"

#include "IRLib.h"

using namespace std;

#define ARP_TABLE_SIZE 256

Settings_t 			Settings;

WiFi_t				WiFi;
WebServer_t 		WebServer;

BLEServerGeneric_t	MyBLEServer;
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

//#define CHIP_DEVICE_CONFIG_ENABLE_WIFI 0
//#define CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP 0

extern "C" void app_main() 
{
	NVS::Init();
	/*
	IRLib::TestAll();
	FreeRTOS::Sleep(5000);
	*/


/*
	IRLib Test;
    Test.RawData 	= vector<int32_t> {2960, -970, 1050, -2820, 1050, -2810, 1050, -2810, 1050, -2820, 1050, -2810, 2960, -970, 1050, -19270, 2990, -940, 1080, -2790, 1050, -2810, 1050, -2820, 1050, -2820, 1050, -2820, 2990, -940, 1050, -19270, 2960, -970, 1050, -2820, 1080, -2790, 1050, -2820, 1050, -2820, 1050, -2810, 2960, -970, 1050, -19270, 2960, -970, 1050, -2820, 1050, -2820, 1050, -2820, 1050, -2820, 1050, -2820, 2960, -980, 1040, -19280, 2960, -980, 1040, -2830, 1040, -2820, 1040, -2830, 1040, -2820, 1040, -2830, 2950, -980, 1040, -19280, 2960, -980, 1040, -2820, 1040, -2830, 1040, -2830, 1040, -2820, 1040, -2830, 2950, -980, 1040, -45000};
    Test.Frequency 	= 38000;
	ESP_LOGE("UP:","%s", Test.GetProntoHex().c_str());

	IRLib Test2;
    Test2.RawData 	= vector<int32_t> {2990, -950, 1050, -2820, 1050, -2810, 1070, -2820, 1050, -2820, 1050, -2810, 1070, -2820, 2960, -17400, 2960, -970, 1050, -2820, 1050, -2820, 1080, -2780, 1090, -2790, 1050, -2810, 1070, -2810, 2980, -17400, 2970, -970, 1070, -2800, 1080, -2780, 1070, -2820, 1040, -2820, 1050, -2820, 1050, -2820, 2960, -17400, 2970, -980, 1040, -2810, 1070, -2820, 1050, -2810, 1070, -2820, 1040, -2810, 1060, -2820, 2960, -17400, 2960, -980, 1040, -2820, 1050, -2820, 1040, -2810, 1070, -2810, 1060, -2820, 1050, -2800, 2980, -17390, 2960, -970, 1050, -2810, 1090, -2790, 1050, -2820, 1050, -2820, 1050, -2820, 1050, -2830, 2960, -45000};
    Test2.Frequency = 38000;
	ESP_LOGE("LEFT:","%s", Test2.GetProntoHex().c_str());


	IRLib Test3;
    Test3.RawData 	= vector<int32_t> {2960, -970, 1050, -2820, 1080, -2780, 1070, -2820, 1050, -2810, 1070, -2820, 2960, -970, 2960, -17400, 2990, -950, 1080, -2780, 1060, -2810, 1070, -2820, 1050, -2810, 1070, -2820, 2960, -970, 2960, -17400, 2960, -980, 1040, -2820, 1050, -2820, 1040, -2820, 1050, -2810, 1090, -2790, 2970, -970, 2960, -17390, 2960, -970, 1070, -2780, 1070, -2820, 1050, -2800, 1060, -2810, 1070, -2820, 2960, -970, 2960, -17390, 2990, -950, 1050, -2820, 1080, -2790, 1050, -2820, 1050, -2820, 1080, -2800, 2960, -970, 2990, -17370, 2960, -970, 1050, -2820, 1070, -2800, 1050, -2820, 1050, -2820, 1050, -2820, 2990, -950, 2990, -45000};
    Test3.Frequency = 38000;
	ESP_LOGE("RIGHT:","%s", Test3.GetProntoHex().c_str());

	IRLib Test4;
    Test4.RawData 	= vector<int32_t> {2960, -970, 1050, -2820, 1050, -2810, 1050, -2820, 1050, -2820, 2990, -940, 1050, -2820, 1050, -19240, 2990, -950, 1070, -2790, 1070, -2790, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2820, 1050, -19240, 2960, -970, 1040, -2810, 1070, -2820, 1050, -2820, 1070, -2800, 2960, -970, 1050, -2820, 1080, -19220, 2990, -950, 1050, -2820, 1050, -2820, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2820, 1050, -19240, 2960, -970, 1050, -2820, 1050, -2820, 1080, -2790, 1050, -2820, 2960, -970, 1050, -2820, 1050, -45000};
    Test4.Frequency = 38000;
	ESP_LOGE("SPOT:","%s", Test4.GetProntoHex().c_str());

	IRLib Test5;
    Test5.RawData 	= vector<int32_t> {2960, -970, 1050, -2820, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2810, 1070, -2820, 1050, -19240, 2990, -950, 1050, -2820, 1050, -2810, 1060, -2810, 2970, -970, 1050, -2820, 1050, -2820, 1050, -19240, 2990, -950, 1050, -2820, 1070, -2790, 1080, -2790, 2960, -970, 1050, -2810, 1060, -2810, 1070, -19240, 2960, -970, 1050, -2820, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2820, 1070, -2790, 1050, -19240, 2960, -970, 1050, -2810, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2810, 1050, -2820, 1050, -19250, 2960, -970, 1050, -2820, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2820, 1050, -2820, 1050, -45000};
    Test5.Frequency = 38000;
	ESP_LOGE("CLEAN:","%s", Test5.GetProntoHex().c_str());

	IRLib Test6;
    Test6.RawData 	= vector<int32_t> {2960, -970, 1050, -2820, 1040, -2810, 1070, -2810, 1060, -2820, 2960, -970, 1050, -2800, 2970, -17390, 2970, -970, 1050, -2820, 1040, -2810, 1060, -2810, 1060, -2820, 2960, -980, 1040, -2810, 2970, -17400, 2960, -980, 1040, -2810, 1060, -2810, 1060, -2810, 1060, -2810, 2970, -980, 1040, -2820, 2960, -17400, 2960, -980, 1040, -2820, 1050, -2820, 1040, -2810, 1060, -2820, 2960, -980, 1040, -2810, 2980, -17390, 2960, -980, 1040, -2820, 1050, -2820, 1050, -2820, 1050, -2820, 2960, -970, 1050, -2820, 2960, -17390, 2960, -970, 1050, -2820, 1050, -2820, 1050, -2820, 1070, -2780, 3000, -950, 1050, -2820, 2980, -45000};
    Test6.Frequency = 38000;
	ESP_LOGE("DOCK:","%s", Test6.GetProntoHex().c_str());
*/
	//esp_log_level_set("*", ESP_LOG_VERBOSE);    // enable WARN logs from WiFi stack

	//!	esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // set warning log level for wifi

	esp_log_level_set("*", ESP_LOG_VERBOSE);      // set warning log level for wifi

	::esp_phy_erase_cal_data_in_nvs(); // clear PHY RF data - tried to do this to make wifi work clearear
	Settings.eFuse.ReadDataOrInit();

	HandlersPooling_t::BARCheck();
	BootAndRestore::OnDeviceStart();

	ESP_LOGI("Current Firmware:", "%s", Settings.Firmware.ToString().c_str());

	Network.ImportScannedSSIDList(WiFi.Scan());

	Device.Init();

	ESP_LOGE("Matter Enabled?","%s", Device.IsMatterEnabled() ? "true" : "false");

	if (Matter::IsEnabledForDevice()) 
		Matter::Init();


    ::Time::SetTimezone();

	Network.Init();
	Automation.Init();

	LocalMQTT.Init();
	RemoteControl.Init();

	Data = DataEndpoint_t::GetForDevice();
	Data->Init();
	Storage.Init();

	WiFi.SetWiFiEventHandler(new MyWiFiEventHandler());

	// Remote temporary hack
	if (Settings.eFuse.Type == Settings.Devices.Remote) {
		GPIO::Setup(GPIO_NUM_22);
		GPIO::Write(GPIO_NUM_22, 0);
	}

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands		= Command_t::GetCommandsForDevice();

	WiFi.SetSTAHostname(Settings.WiFi.APSSID);

	if (Matter::IsEnabledForDevice())
		Matter::StartServer();

	MyBLEServer.StartAdvertising();

	if (Network.WiFiSettings.size() == 0) // first start for HID devices
		MyBLEServer.ForceHIDMode(BASIC);

	WebServer.HTTPStart();

	HandlersPooling_t::Pool();

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}
