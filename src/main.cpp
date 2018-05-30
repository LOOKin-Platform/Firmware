using namespace std;

#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <esp_heap_trace.h>

#include "Globals.h"

#include "Log.h"
#include "UART.h"
#include "WiFi.h"
#include "WiFiEventHandler.h"
#include "FreeRTOSWrapper.h"
#include "DateTime.h"
#include "Memory.h"
#include "Sleep.h"

#include "handlers/OverheatHandler.cpp"
#include "handlers/WiFiHandler.cpp"

#include "I2C.h"

#include "esp_efuse.h"


extern "C" {
	void app_main(void);
}

Settings_t			Settings;

WiFi_t				WiFi;
WebServer_t 		WebServer;
BluetoothServer_t	BluetoothServer;

Device_t			Device;
Network_t			Network;
Automation_t		Automation;
Storage_t			Storage;

vector<Sensor_t*>	Sensors;
vector<Command_t*>	Commands;

static char tag[] = "Main";

void app_main(void) {
	uint32_t StartMemory = system_get_free_heap_size();

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

	Network.Init();
	Automation.Init();

	Sensors 		= Sensor_t::GetSensorsForDevice();
	Commands		= Command_t::GetCommandsForDevice();

	WiFi.addDNSServer("8.8.8.8");
	WiFi.addDNSServer("8.8.4.4");
	WiFi.setWifiEventHandler(new MyWiFiEventHandler());

	if (!Network.WiFiConnect())
		WiFi.StartAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);

	WebServer.Start();
	BluetoothServer.Start();

	Log::Add(LOG_DEVICE_STARTED);

	I2C bus;
	bus.Init(0x48);

	for (int i = 0x92; i < 0x120; i++) {
		SPIFlash::EraseSector(i);
	}

	while (1) {
		ESP_LOGI("main","RAM left %d (Started: %d)", system_get_free_heap_size(), StartMemory);

		OverheatHandler::Pool(Settings.Pooling.Interval);

		/*
		bus.BeginTransaction();
	    uint8_t* byte = (uint8_t*) malloc(2);
		bus.Read(byte, 2, false);
		bus.EndTransaction();

		float temp = byte[0];

		if (byte[0] >= 0x80)
			temp = - (temp - 0x80);

		if (byte[1] == 0x80)
			temp += 0.5;

		float ChipTemperature = temprature_sens_read();
		ChipTemperature = (ChipTemperature - 32) * (5.0/9.0) + 0.5;

		ESP_LOGI("main","I2C temp: %f, Chip temp: %f", temp, ChipTemperature);
		free(byte);
		*/


		vTaskDelay(Settings.Pooling.Interval);
	}

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
}

Settings_t::eFuse_t::eFuse_t() {
	uint32_t eFuseData = 0x00;

	eFuseData = REG_READ(EFUSE_BLK3_RDATA7_REG);
	Type 				= (uint8_t)(eFuseData >> 16);
	Revision 			= (uint16_t)((eFuseData << 16) >> 16);

	eFuseData = REG_READ(EFUSE_BLK3_RDATA6_REG);
	Model 				= (uint8_t)eFuseData >> 24;
	DeviceID 			= (uint32_t)((eFuseData << 8) >> 8);

	eFuseData = REG_READ(EFUSE_BLK3_RDATA5_REG);
	DeviceID 			= (DeviceID << 8) + (uint8_t)(eFuseData >> 24);
	Misc				= (uint8_t)((eFuseData << 8) >> 24);
	Produced.Month		= Converter::InterpretHexAsDec((uint8_t)((eFuseData << 16) >> 24));
	Produced.Day		= Converter::InterpretHexAsDec((uint8_t)((eFuseData << 24) >> 24));

	eFuseData = REG_READ(EFUSE_BLK3_RDATA4_REG);
	Produced.Year		= (uint16_t)(eFuseData >> 16);
	Produced.Year 		= Converter::InterpretHexAsDec(Produced.Year);

	Produced.Factory	= (uint16_t)((eFuseData << 16) >> 24);
	Produced.Destination= (uint16_t)((eFuseData << 24) >> 24);

	// Type verification
	if (Type == 0x0 || Type == Settings.Memory.Empty8Bit) {
		Type = Device.GetTypeFromNVS();

		if (Type == 0x0 || Type == 0xFF) {
			Type = 0x81;
			Device.SetTypeToNVS(Type);
		}
	}
	else
		Device.SetTypeToNVS(Type);


	// DeviceID verification
	if (DeviceID == 0x0 || DeviceID == Settings.Memory.Empty32Bit) {
		DeviceID = Device.GetIDFromNVS();

		if (DeviceID == 0x0 || DeviceID == Settings.Memory.Empty32Bit) {
			DeviceID = Device.GenerateID();
			Device.SetIDToNVS(DeviceID);
		}
	}
	else
		Device.SetIDToNVS(DeviceID);

}
