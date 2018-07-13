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
	Remote.Color.Timer			= LEDC_TIMER_0;
	Remote.Color.Red.GPIO		= GPIO_NUM_12;
	Remote.Color.Red.Channel	= LEDC_CHANNEL_0;
	Remote.Color.Green.GPIO		= GPIO_NUM_14;
	Remote.Color.Green.Channel	= LEDC_CHANNEL_1;
	Remote.Color.Blue.GPIO		= GPIO_NUM_13;
	Remote.Color.Blue.Channel	= LEDC_CHANNEL_2;

	Remote.IR.ReceiverGPIO38	= GPIO_NUM_22;
	Remote.IR.ReceiverGPIO56	= GPIO_NUM_4;
	Remote.IR.SenderGPIO		= GPIO_NUM_23;

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

	FillDevices();
}
