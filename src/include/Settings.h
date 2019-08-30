/*
*    Settings.h
*    General firmware settings
*
*/
#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include "esp_efuse.h"
#include "soc/efuse_reg.h"

#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/timer.h"
#include "esp_bt.h"

#include "Converter.h"

using namespace std;

#define WIFI_AP_NAME		"LOOK.in_" + Device.TypeToString() + "_" + Device.IDToString()
#define WIFI_AP_PASSWORD    Device.IDToString()

class Settings_t {
	public:
		const	string 						FirmwareVersion = "1.24";

		struct {
			const string					APIUrl 			= "http://download.look-in.club/firmwares/";
			const uint16_t					ServerPort		= 80;
			const uint8_t					MaxAttempts		= 3;
		} OTA;

		struct {
			const string 					APIUrl 			= "http://api.look-in.club/v1/time";
		} TimeSync;

		struct WiFi_t {
			const uint32_t					IPCountdown 	= 90000;
			const uint8_t					SavedAPCount	= 8;
			const string 					DefaultIP		= "192.168.4.1";

			static constexpr uint16_t		UPDPort			= 61201;
			static constexpr uint8_t		UDPHoldPortsMax	= 8;
			const string					UDPPacketPrefix	= "LOOK.in:";

			uint32_t						BatteryUptime	= 120* 1000; 	// first time WiFi time to work, ms
			uint32_t						KeepWiFiTime	= 60 * 1000;	// Keep wifi after /network/KeepWiFi command executing, ms

			uint32_t						STAModeInterval = 300;	// Interval to check if any existed clients nearby, sec
			uint32_t						STAModeReconnect= 60;	// Interval to check if any existed clients nearby after loosing connection, sec

			struct UDPBroadcastQueue_t {
				static constexpr uint8_t	Size			= 10;
				static constexpr uint8_t	BlockTicks		= 50;
				static constexpr uint8_t	MaxMessageSize	= 64;
				static constexpr uint16_t	Pause			= 100; // pause between UDP packets in ms
				static constexpr uint8_t	CheckGap		= 5;   // if packet was scheduled and time expired - don't send. Time in seconds.
			} UDPBroadcastQueue;

			static constexpr uint16_t		HTTPMaxQueryLen	= 6144;

			struct {
				uint32_t Count		= 4;
				uint32_t Timeout	= 1000;
				uint32_t Delay		= 500;
			} PingAfterConnect;

		} WiFi;

		struct Bluetooth_t {
			const string					DeviceNamePrefix		= "LOOK.in ";
			const string					SecretCodeServiceUUID   = "997d2872-b4e2-414d-b4be-985263a70411";
        	const string 					SecretCodeUUID    		= "1d9fe7b3-5633-4b98-8af4-1bbf2d7c50ea";

        	const esp_power_level_t			PublicModePower			= ESP_PWR_LVL_P6;
        	const esp_power_level_t			PrivateModePower		= ESP_PWR_LVL_N0;//ESP_PWR_LVL_N3;
		} Bluetooth;

		struct MQTT_t {
			const string 					ServerHost				= "mqtts://mqtt.look-in.club:8883";
			const uint8_t					MaxConnectionTries		= 3;

			const uint8_t					DefaultQOS				= 2;
			const uint8_t					DefaultRetain			= 0;

			const string					DeviceTopicPrefix		= "/devices/";
		} MQTT;

		struct Wireless_t {
			map<uint8_t, pair<uint16_t,uint16_t>> AliveIntervals = {
				{ 0x8, { 294, 6 }}
			};

			const uint8_t					IntervalID 		= 0x8;
		} Wireless;

		struct {
			const uint8_t 					SystemLogSize	= 16;
			const uint8_t 					EventsLogSize	= 32;
		} Log;

		struct {
			const uint16_t					Interval 		= 1000;
			struct {
				const uint16_t				Inverval 		= 4000;
				const uint8_t 				OverheatTemp	= 90;
				const uint8_t				ChilledTemp		= 77;
			} OverHeat;
		} Pooling;

		struct Devices_t {
			public:
				static constexpr uint8_t	Plug 			= 0x03;
				static constexpr uint8_t	Remote			= 0x81;
				static constexpr uint8_t	Motion			= 0x82;

				map<uint8_t,string> Literaly = {
					{ Plug	, "Plug"	},
					{ Remote, "Remote" 	},
					{ Motion, "Motion" 	}
				};
		} Devices;

		struct eFuse_t {
			public:
				uint8_t 		Type 		= 0x00; //Expandable for uint16_t
				uint16_t		Revision	= 0x00;
				uint8_t			Model		= 0x00;
				uint32_t		DeviceID	= 0x00;
				uint8_t			Misc		= 0x00;

				struct Produced_t {
					uint8_t 	Month 		= 0x00;
					uint8_t 	Day 		= 0x00;
					uint16_t	Year 		= 0x00;
					uint8_t		Factory		= 0x00;
					uint8_t		Destination	= 0x00;
				} Produced;

				void ReadData();
				uint32_t ReverseBytes(uint32_t);
		} eFuse;

		struct HTTPClient_t {
			static constexpr uint16_t 		QueueSize		= 16;
			static constexpr uint8_t		ThreadsMax		= 4;
			static constexpr uint8_t		BlockTicks		= 50;
			static constexpr uint16_t		NetbuffLen		= 512;
			static constexpr uint8_t		NewThreadStep	= 2;
			static constexpr uint16_t		ThreadStackSize = 4096;
		} HTTPClient;

		struct Memory_t {
			static constexpr uint8_t		Empty8Bit		= 0xFF;
			static constexpr uint16_t		Empty16Bit		= 0xFFFF;
			static constexpr uint32_t		Empty32Bit		= 0xFFFFFFFF;

			static constexpr uint32_t		BlockSize		= 0x1000;
		} Memory;

		struct Storage_t {
			struct Versions_t {
				static constexpr uint32_t 	StartAddress	= 0x92000;
				static constexpr uint32_t 	Size			= 0xA000;
				static constexpr uint8_t 	VersionMaxSize	= 20;
				static constexpr uint16_t 	VersionsMax		= 0x400;
			} Versions;

			struct Data_t {
				static constexpr uint32_t 	StartAddress	= 0x9C000;
				static constexpr uint32_t 	Size			= 0x84000;
				static constexpr uint16_t 	ItemSize		= 0x108;
			} Data;
		} Storage;

		struct Scenarios_t {
			static constexpr uint8_t		QueueSize		= 64;
			static constexpr uint16_t		BlockTicks		= 50;
			static constexpr uint16_t		TaskStackSize	= 8192;

			static constexpr uint16_t		OperandBitLength= 64;
			static constexpr uint8_t		NameLength		= 64;
			static constexpr uint8_t		CacheBitLength	= 32;

			static constexpr uint8_t		TimePoolInterval= 5;

			struct Types_t {
				static constexpr uint8_t	EventHex		= 0x00;
				static constexpr uint8_t	TimerHex		= 0x01;
				static constexpr uint8_t	CalendarHex		= 0x02;

				static constexpr uint8_t	EmptyHex		= 0xFF;
			} Types;

			struct Memory_t {
				static constexpr uint32_t	Start			= 0x32000;
				static constexpr uint32_t	Size			= 0x60000;
				static constexpr uint32_t	ItemSize		= 0x600;	// 1536 байт
				static constexpr uint16_t	Count			= 256;		// 1536 байт

				struct ItemOffset_t {
					static constexpr uint16_t ID			= 0x0;
					static constexpr uint16_t Type			= 0x4;
					static constexpr uint16_t Operand		= 0x6;
					static constexpr uint16_t Name			= 0xE;
					static constexpr uint16_t Commands		= 0x90;
				} ItemOffset;

				struct CommandOffset_t {
					static constexpr uint16_t Size			= 0xA;

					static constexpr uint16_t DeviceID		= 0x0;
					static constexpr uint16_t CommandID		= 0x4;
					static constexpr uint16_t EventID		= 0x5;
					static constexpr uint16_t Operand		= 0x6;
				} CommandOffset;
			} Memory;
		} Scenarios;

		// Commands and sensors data

		struct GPIOData_t {
			struct Switch_t {
				gpio_num_t			GPIO 			= GPIO_NUM_0;
			};

			struct Color_t {
				struct Item_t {
					gpio_num_t		GPIO 			= GPIO_NUM_0;
					ledc_channel_t	Channel 		= LEDC_CHANNEL_MAX;
				};

				ledc_timer_t		Timer			= LEDC_TIMER_MAX;
				Item_t				Red, Green, Blue, White;
			};

			struct IR_t {
				gpio_num_t			ReceiverGPIO38	= GPIO_NUM_0;
				gpio_num_t			ReceiverGPIO56	= GPIO_NUM_0;
				gpio_num_t			SenderGPIO		= GPIO_NUM_0;
			};

			struct Temperature_t {
				uint8_t				I2CAddress		= 0x00;
			};

			struct Motion_t {
				uint32_t			PoolInterval	= 50;
				adc1_channel_t		ADCChannel		= ADC1_CHANNEL_MAX;
			};

			struct Indicator_t {
				ledc_timer_t		Timer	= LEDC_TIMER_MAX;
				Settings_t::GPIOData_t::Color_t::Item_t
									Red, Green, Blue, White;

				timer_group_t 		ISRTimerGroup = TIMER_GROUP_MAX;
				timer_idx_t			ISRTimerIndex = TIMER_MAX;
			};

			struct PowerMeter_t {
				adc1_channel_t 		ConstPowerChannel 	= ADC1_CHANNEL_MAX;
				adc1_channel_t 		BatteryPowerChannel = ADC1_CHANNEL_MAX;
			};

			struct DeviceInfo_t {
				Switch_t		Switch;
				Color_t			Color;
				IR_t			IR;
				Motion_t		Motion;
				Temperature_t	Temperature;

				// Device modes indicator
				Indicator_t		Indicator;

				// Device PowerMeters
				PowerMeter_t 	PowerMeter;
			};

			DeviceInfo_t GetCurrent();

			static map<uint8_t, DeviceInfo_t> Devices;

			/* Implementation of this settings see Settings.cpp */

		} GPIOData;
};

extern Settings_t Settings;

#endif
