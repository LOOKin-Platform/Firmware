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

#include "nimble/nimble_port_freertos.h"

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
	NVS Memory1("chip-factory");
	Memory1.EraseNamespace();

	NVS Memory2("chip-config");
	Memory2.EraseNamespace();

	NVS Memory3("chip-counters");
	Memory3.EraseNamespace();
	*/

    /*
	IRLib Test;
    Test.RawData = 	vector<int32_t> {9024, -4512, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -19000, 9024, -4512, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -19000, 9024, -4512, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -1692, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -45000};
    Test.Frequency = 40000;
*/
//	ESP_LOGE("Signal:","%s", Test.GetProntoHex().c_str());

    IRLib Test("000F006700660000016800B40016004300160016001600430016001600160016001600430016001600160043001600160016004300160016001600430016004300160016001600430016001600160016001600160016004300160043001600430016001600160016001600160016004300160043001600160016001600160016001600430016004300160043001602F8016800B40016004300160016001600430016001600160016001600430016001600160043001600160016004300160016001600430016004300160016001600430016001600160016001600160016004300160043001600430016001600160016001600160016004300160043001600160016001600160016001600430016004300160043001602F8016800B4001600430016001600160043001600160016001600160043001600160016004300160016001600430016001600160043001600430016001600160043001600160016001600160016001600430016004300160043001600160016001600160016001600430016004300160016001600160016001600160043001600430016004300160708");
	ESP_LOGE("Signal:","%s", Test.GetRawSignal().c_str());

	//esp_log_level_set("*", ESP_LOG_VERBOSE);    // enable WARN logs from WiFi stack
	esp_log_level_set("wifi", ESP_LOG_DEBUG);      // set warning log level for wifi

	//!
	esp_log_level_set("*", ESP_LOG_INFO);      // set warning log level for wifi

	::esp_phy_erase_cal_data_in_nvs(); // clear PHY RF data - tried to do this to make wifi work clearear
	Settings.eFuse.ReadDataOrInit();

	Pooling_t::BARCheck();
	BootAndRestore::OnDeviceStart();

	ESP_LOGI("Current Firmware:", "%s", Settings.Firmware.ToString().c_str());

	Network.ImportScannedSSIDList(WiFi.Scan());

	if (Matter::IsEnabledForDevice()) 
		Matter::Init();

	Time::SetTimezone();

	Device.Init();
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

	BLEServer.StartAdvertising();

	if (Network.WiFiSettings.size() == 0) // first start for HID devices
		BLEServer.ForceHIDMode(BASIC);

	WebServer.HTTPStart();

	Pooling_t::Pool();

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/
}
