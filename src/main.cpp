
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
#include "UART.h"
#include "WiFi.h"
#include "WiFiEventHandler.h"
#include "FreeRTOSWrapper.h"
#include "DateTime.h"
#include "Sleep.h"

#include "handlers/OverheatHandler.cpp"
#include "handlers/WiFiHandler.cpp"

void ADCTest();

extern "C" {
	void app_main(void);
}

WiFi_t				*WiFi				= new WiFi_t();
WebServer_t 			*WebServer 			= new WebServer_t();
BluetoothServer_t	*BluetoothServer 	= new BluetoothServer_t();

Device_t				*Device		= new Device_t();
Network_t			*Network		= new Network_t();
Automation_t			*Automation	= new Automation_t();

vector<Sensor_t*>	Sensors;
vector<Command_t*>	Commands;

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    		ESP_LOGE("main", "Error while NVS flash init, %d", err);

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
	Commands		= Command_t::GetCommandsForDevice();

	WiFi->addDNSServer("8.8.8.8");
	WiFi->addDNSServer("8.8.4.4");
	WiFi->setWifiEventHandler(new MyWiFiEventHandler());

	if (!Network->WiFiConnect())
		WiFi->StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

	WebServer->Start();
	//BluetoothServer->Start();

	Log::Add(LOG_DEVICE_STARTED);

	OverheatHandler::Start();

	//IRLib IRSignal("0000 006C 0022 0002 015B 00AD 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 064D 015B 0057 0016 0E6C");

	//ESP_LOGD("IRLib", "Frequency: %s", Converter::ToString(IRSignal.Frequency).c_str());
	//ESP_LOGD("IRLib", "Pronto:	 %s", IRSignal.GetProntoHex().c_str());

	//for (int i=0; i<IRSignal.RawData.size(); i++)
	//	ESP_LOGI("SIGNAL","%d",IRSignal.RawData[i]);
	//ESP_LOGI("Uint32Data", "%X", IRSignal.Uint32Data);

	//GPIO::Setup(GPIO_NUM_26);
	//GPIO::Write(GPIO_NUM_26, true);

	/*
	while (1) {
		Command_t::GetCommandByID(0x07)->Execute(0x1,"AE00");
		//Command_t::GetCommandByID(0x07)->Execute(0x1,0x082000);
		vTaskDelay(2000);
	}
	*/

	/*
	if (Sleep::GetWakeUpReason() != ESP_SLEEP_WAKEUP_EXT0) {
		Sleep::SetGPIOWakeupSource(IR_REMOTE_RECEIVER_GPIO,0);
		//Sleep::SetTimerInterval(10);
		Sleep::LightSleep();
	}
	*/

    //ADC test
	//ADCTest();
}
