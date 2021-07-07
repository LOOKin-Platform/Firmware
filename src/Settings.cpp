/*
*    Settings.cpp
*    General firmware settings handlers and class functions
*
*/

#include "Settings.h"
#include "BootAndRestore.h"
#include "Globals.h"

map<uint8_t, Settings_t::GPIOData_t::DeviceInfo_t> Settings_t::GPIOData_t::Devices = {};

void FillDevices() {
	Settings_t::GPIOData_t::DeviceInfo_t Plug;
	Plug.Switch.GPIO						= GPIO_NUM_23;

	Plug.Color.Timer						= LEDC_TIMER_0;
	Plug.Color.Blue.GPIO					= GPIO_NUM_2;
	Plug.Color.Blue.Channel					= LEDC_CHANNEL_2;

	Settings_t::GPIOData_t::DeviceInfo_t Duo;
	Duo.MultiSwitch.GPIO 					= { GPIO_NUM_4, GPIO_NUM_17 };
	Duo.Touch.GPIO							= { GPIO_NUM_18 };

	Duo.Indicator.Timer						= LEDC_TIMER_0;
	Duo.Indicator.Red.GPIO					= GPIO_NUM_0;
	Duo.Indicator.Red.Channel				= LEDC_CHANNEL_0;
	Duo.Indicator.Green.GPIO				= GPIO_NUM_25;
	Duo.Indicator.Green.Channel				= LEDC_CHANNEL_1;
	Duo.Indicator.Blue.GPIO					= GPIO_NUM_0;
	Duo.Indicator.Blue.Channel				= LEDC_CHANNEL_2;
	Duo.Indicator.ISRTimerGroup				= TIMER_GROUP_0;
	Duo.Indicator.ISRTimerIndex				= TIMER_0;

	Settings_t::GPIOData_t::DeviceInfo_t Remote;
	Remote.Indicator.Timer					= LEDC_TIMER_0;
	Remote.Indicator.Red.GPIO				= GPIO_NUM_25;
	Remote.Indicator.Red.Channel			= LEDC_CHANNEL_0;
	Remote.Indicator.Green.GPIO				= GPIO_NUM_12;
	Remote.Indicator.Green.Channel			= LEDC_CHANNEL_1;
	Remote.Indicator.Blue.GPIO				= GPIO_NUM_13;
	Remote.Indicator.Blue.Channel			= LEDC_CHANNEL_2;
	Remote.Indicator.ISRTimerGroup			= TIMER_GROUP_0;
	Remote.Indicator.ISRTimerIndex			= TIMER_0;

	Remote.PowerMeter.ConstPowerChannel		= ADC1_CHANNEL_5;
	Remote.PowerMeter.BatteryPowerChannel	= ADC1_CHANNEL_4;

	Remote.IR.ReceiverGPIO38				= GPIO_NUM_26;
	Remote.IR.ReceiverGPIO56				= GPIO_NUM_27;

	Remote.IR.SenderGPIOExt 				= GPIO_NUM_0;

	if (Settings.eFuse.Model > 0) {
		Remote.IR.SenderGPIOs				= { GPIO_NUM_14, GPIO_NUM_27, GPIO_NUM_16 };

		if (Settings.eFuse.Model == 2)
			Remote.IR.SenderGPIOExt = GPIO_NUM_4;
	}
	else
		Remote.IR.SenderGPIOs				= { GPIO_NUM_4 };

	if (Settings.eFuse.Revision == 0x01)
		Remote.Temperature.I2CAddress 		= 0x48;

	Settings_t::GPIOData_t::DeviceInfo_t Motion;
	Motion.Motion.PoolInterval				= 50;
	Motion.Motion.ADCChannel				= ADC1_CHANNEL_3;

	Settings_t::GPIOData_t::Devices =
	{
		{ 0x02, Duo 	},
		{ 0x03, Plug 	},
		{ 0x81, Remote 	},
		{ 0x82, Motion 	}
	};
}

Settings_t::GPIOData_t::DeviceInfo_t Settings_t::GPIOData_t::GetCurrent() {
	uint8_t Type = Settings.eFuse.Type;

	if (Settings.GPIOData.Devices.count(Type))
		return Settings.GPIOData.Devices[Type];
	else {
		DeviceInfo_t Data;
		return Data;
	}
}

uint32_t Settings_t::eFuse_t::ReverseBytes(uint32_t Value) {
	uint32_t Result = 0x0;
	Result += (Value & 0x000000ff)			<< 24;
	Result += ((Value & 0x0000ff00) >> 8) 	<< 16;
	Result += ((Value & 0x00ff0000) >> 16) 	<< 8;
	Result += ((Value & 0xff000000) >> 24);

	return Result;
}

void Settings_t::eFuse_t::ReadDataOrInit() {
	uint32_t eFuseData1, eFuseData2, eFuseData3, eFuseData4 = 0x00;

	eFuseData1 = esp_efuse_read_reg(EFUSE_BLK3, 7);
	eFuseData2 = esp_efuse_read_reg(EFUSE_BLK3, 6);
	eFuseData3 = esp_efuse_read_reg(EFUSE_BLK3, 5);
	eFuseData4 = esp_efuse_read_reg(EFUSE_BLK3, 4);

	if (eFuseData1 + eFuseData2 + eFuseData3 + eFuseData4 == 0x0) {
		eFuseData1 = esp_efuse_read_reg(EFUSE_BLK3, 0);
		eFuseData2 = esp_efuse_read_reg(EFUSE_BLK3, 1);
		eFuseData3 = esp_efuse_read_reg(EFUSE_BLK3, 2);
		eFuseData4 = esp_efuse_read_reg(EFUSE_BLK3, 3);
	}

	//check is valid efuse or not - different SOC versions has different storage schemes
	uint16_t TestYear = (uint16_t)(eFuseData4 >> 16);

	if (TestYear < 2018 || TestYear > 2100)
	{
		eFuseData1 = ReverseBytes(eFuseData1);
		eFuseData2 = ReverseBytes(eFuseData2);
		eFuseData3 = ReverseBytes(eFuseData3);
		eFuseData4 = ReverseBytes(eFuseData4);
	}

	if (eFuseData1 + eFuseData2 + eFuseData3 + eFuseData4 == 0x0)
		InitFromNVS();

	Type 				= (uint8_t)(eFuseData1 >> 16);

	Revision 			= (uint16_t)((eFuseData1 << 16) >> 16);

	Model 				= (uint8_t)(eFuseData2 >> 24);

	DeviceID 			= (uint32_t)((eFuseData2 << 8) >> 8);
	DeviceID 			= (DeviceID << 8) + (uint8_t)(eFuseData3 >> 24);

	Misc				= (uint8_t)((eFuseData3 << 8) >> 24);

	Produced.Month		= Converter::InterpretHexAsDec((uint8_t)((eFuseData3 << 16) >> 24));
	Produced.Day		= Converter::InterpretHexAsDec((uint8_t)((eFuseData3 << 24) >> 24));
	Produced.Year		= (uint16_t)(eFuseData4 >> 16);
	Produced.Year 		= Converter::InterpretHexAsDec(Produced.Year);

	Produced.Factory	= (uint16_t)((eFuseData4 << 16) >> 24);

	Produced.Destination= (uint16_t)((eFuseData4 << 24) >> 24);

	// workaround for 00000001 remote device
	if (Type == 0 && DeviceID == 0x2 && Revision == 0x8100 && Produced.Year == 700)
	{
		Revision		= 0;
		Model			= 0;
		Type 			= 0x81;
		DeviceID 		= 0x00000001;
		Produced.Day 	= 0x1;
		Produced.Month 	= 0x6;
		Produced.Year	= 2018;
	}

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

	if (DeviceID == 0x00000002)
		Revision = 0x1;

	// workaround for Remote2 test batch
	// Model = 2;

	// Setup device generation
	if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model < 2)
		Settings.DeviceGeneration = 1;
	else
		Settings.DeviceGeneration = 2;

	FillDevices();

	Settings.WiFi.APSSID 			= "LOOKin_" + Device.IDToString();
	Settings.WiFi.APPassword		= Device.IDToString();
}

void Settings_t::eFuse_t::InitFromNVS() {
	string Partition = "factory_nvs";

	NVS::Init(Partition);
	//NVS::DeInit("factory_nvs");

	NVS Memory(Partition, "init", NVS_READONLY);

	if (Memory.GetLastError() != ESP_OK)
		return;

	Log::Add(0x00F1);

	uint32_t 	DeviceID 		= Converter::UintFromHexString<uint32_t>(Memory.GetString("id"));
	uint8_t 	DeviceType 		= Memory.GetInt8Bit("type");
	uint8_t 	DeviceModel 	= Memory.GetInt8Bit("model");
	uint8_t 	DeviceRevision 	= Memory.GetInt8Bit("revision");
	uint8_t 	Misc 			= Memory.GetInt8Bit("misc");

	uint8_t 	Manufactorer 	= Memory.GetInt8Bit("manufactorer");
	uint8_t 	Country 		= Memory.GetInt8Bit("country");
	string 		Date			= Memory.GetString("date");

	string 		HKSetupID		= Memory.GetString("hksetupid");
	string 		HKPinCode		= Memory.GetString("hkpincode");

	uint32_t eFuseData1 = 0;
	uint32_t eFuseData2 = 0;
	uint32_t eFuseData3 = 0;
	uint32_t eFuseData4 = 0;

	//00810001
	eFuseData1 = ((uint32_t)DeviceType) << 16;
	eFuseData1 = eFuseData1 + DeviceRevision;
	eFuseData1 = ReverseBytes(eFuseData1);

	eFuseData2 = ((uint32_t)DeviceModel) << 24;
	eFuseData2 = (eFuseData2 + (DeviceID >> 8));
	eFuseData2 = ReverseBytes(eFuseData2);

	eFuseData3 = (DeviceID << 24);
	eFuseData3 = eFuseData3 + (((uint32_t)Misc) << 16);

	vector<string> DateArray = Converter::StringToVector(Date, ".");
	uint32_t Month 	= ((uint32_t)Converter::UintFromHexString<uint8_t>(DateArray[1])) << 8;
	uint32_t Day 	= ((uint32_t)Converter::UintFromHexString<uint8_t>(DateArray[0]));

	eFuseData3 		= ReverseBytes(eFuseData3 + Month + Day);

	uint32_t Year 	= Converter::UintFromHexString<uint32_t>(DateArray[2]);
	eFuseData4 		= (Year << 16);
	eFuseData4 		= eFuseData4 + (Manufactorer << 8);
	eFuseData4 		= eFuseData4 + Country;
	eFuseData4		= ReverseBytes(eFuseData4);

	//Записываем в eFuse

	esp_efuse_write_reg(EFUSE_BLK3, 7, eFuseData1);
	esp_efuse_write_reg(EFUSE_BLK3, 6, eFuseData2);
	esp_efuse_write_reg(EFUSE_BLK3, 5, eFuseData3);
	esp_efuse_write_reg(EFUSE_BLK3, 4, eFuseData4);
	esp_efuse_set_write_protect(EFUSE_BLK3);

	::nvs_flash_erase_partition(Partition.c_str());

	NVS::Init(Partition);

	if (HKSetupID != "" && HKPinCode != "")
	{
		NVS MemoryHAP(Partition, "hap_custom");
		MemoryHAP.SetString("setupid", HKSetupID);
		MemoryHAP.SetString("pin", HKPinCode);
		MemoryHAP.Commit();
	}

	Settings.DeviceGeneration = 2;

	BootAndRestore::HardResetFromInit();

	FreeRTOS::Sleep(60000);
}

