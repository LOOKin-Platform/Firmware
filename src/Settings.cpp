/*
*    Settings.cpp
*    General firmware settings handlers and class functions
*
*/

#include "Settings.h"
#include "Globals.h"

map<uint8_t, Settings_t::GPIOData_t::DeviceInfo_t> Settings_t::GPIOData_t::Devices = {};

void FillDevices() {
	Settings_t::GPIOData_t::DeviceInfo_t Plug;
	Plug.Switch.GPIO			= GPIO_NUM_23;

	Plug.Color.Timer			= LEDC_TIMER_0;
	Plug.Color.Blue.GPIO		= GPIO_NUM_2;
	Plug.Color.Blue.Channel		= LEDC_CHANNEL_2;

	Settings_t::GPIOData_t::DeviceInfo_t Remote;
	Remote.Indicator.Timer			= LEDC_TIMER_0;
	Remote.Indicator.Red.GPIO		= GPIO_NUM_25;
	Remote.Indicator.Red.Channel	= LEDC_CHANNEL_0;
	Remote.Indicator.Green.GPIO		= GPIO_NUM_12;
	Remote.Indicator.Green.Channel	= LEDC_CHANNEL_1;
	Remote.Indicator.Blue.GPIO		= GPIO_NUM_13;
	Remote.Indicator.Blue.Channel	= LEDC_CHANNEL_2;
	Remote.Indicator.ISRTimerGroup	= TIMER_GROUP_0;
	Remote.Indicator.ISRTimerIndex	= TIMER_0;

	Remote.IR.ReceiverGPIO38	= GPIO_NUM_26;
	Remote.IR.ReceiverGPIO56	= GPIO_NUM_27;
	Remote.IR.SenderGPIO		= GPIO_NUM_4;

	Settings_t::GPIOData_t::DeviceInfo_t Motion;
	Motion.Motion.PoolInterval	= 50;
	Motion.Motion.ADCChannel	= ADC1_CHANNEL_3;

	Settings_t::GPIOData_t::Devices =
	{
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


void Settings_t::eFuse_t::ReadData() {
	uint32_t eFuseData1, eFuseData2, eFuseData3, eFuseData4 = 0x00;

	eFuseData1 = REG_READ(EFUSE_BLK3_RDATA7_REG);
	eFuseData2 = REG_READ(EFUSE_BLK3_RDATA6_REG);
	eFuseData3 = REG_READ(EFUSE_BLK3_RDATA5_REG);
	eFuseData4 = REG_READ(EFUSE_BLK3_RDATA4_REG);

	if (eFuseData1 + eFuseData2 + eFuseData3 + eFuseData4 == 0x0) {
		eFuseData1 = REG_READ(EFUSE_BLK3_RDATA0_REG);
		eFuseData2 = REG_READ(EFUSE_BLK3_RDATA1_REG);
		eFuseData3 = REG_READ(EFUSE_BLK3_RDATA2_REG);
		eFuseData4 = REG_READ(EFUSE_BLK3_RDATA3_REG);
	}

	Type 				= (uint8_t)(eFuseData1 >> 16);
	Revision 			= (uint16_t)((eFuseData1 << 16) >> 16);

	Model 				= (uint8_t)eFuseData2 >> 24;
	DeviceID 			= (uint32_t)((eFuseData2 << 8) >> 8);

	DeviceID 			= (DeviceID << 8) + (uint8_t)(eFuseData3 >> 24);
	Misc				= (uint8_t)((eFuseData3 << 8) >> 24);
	Produced.Month		= Converter::InterpretHexAsDec((uint8_t)((eFuseData3 << 16) >> 24));
	Produced.Day		= Converter::InterpretHexAsDec((uint8_t)((eFuseData3 << 24) >> 24));

	Produced.Year		= (uint16_t)(eFuseData4 >> 16);
	Produced.Year 		= Converter::InterpretHexAsDec(Produced.Year);

	Produced.Factory	= (uint16_t)((eFuseData4 << 16) >> 24);
	Produced.Destination= (uint16_t)((eFuseData4 << 24) >> 24);

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

	FillDevices();
}
