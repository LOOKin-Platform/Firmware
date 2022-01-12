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
#include "esp_bt.h"


#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/timer.h"

#include "Converter.h"

using namespace std;

#define UINT32MAX 0xFFFFFFFF

typedef struct FirmwareVersionStruct {
	union __attribute__((__packed__)) {
		struct { uint8_t Major : 8, Minor : 8; uint16_t Revision : 16; };
		uint32_t Uint32Data = 0x00000000;
	} Version;

	FirmwareVersionStruct(uint32_t sVersion) { Version.Uint32Data = sVersion; }
	FirmwareVersionStruct(uint8_t sMajor, uint8_t sMinor, uint16_t sRevision) { Version.Major = sMajor; Version.Minor = sMinor; Version.Revision = sRevision;}
	string 	ToString() { return (Version.Uint32Data == 0) ? "" : Converter::ToString<uint8_t>(Version.Major) + "." + Converter::ToString<uint8_t>(Version.Minor,2)+ "." + Converter::ToString<uint16_t>(Version.Revision,4);}
	bool 	operator== (const FirmwareVersionStruct &fw2) { return (Version.Uint32Data == fw2.Version.Uint32Data); }
	bool 	operator!= (const FirmwareVersionStruct &fw2) { return !(Version.Uint32Data == fw2.Version.Uint32Data); }
} FirmwareVersion;


class Settings_t {
	public:
		FirmwareVersion 					Firmware = FirmwareVersion(2, 41, 0001);

//		const FirmwareVersion Firmware =  0x020A0000;

		/*
		struct {
			const FirmwareVersion			Version			= 0x020A0000;


			string 			ToString() 						{ return Converter::ToString<uint8_t>(Major) + "." + Converter::ToString<uint8_t>(Minor,2);}
			uint32_t 		ToUint32() 						{ return (((uint32_t)Major) << 24) + (((uint32_t)Minor) << 16) + Revision; }
			static string 	Uint32ToString(uint32_t Version){ return Converter::ToString((uint8_t)(Version >> 24)) + "." + Converter::ToString((uint8_t)((Version << 8) >> 24), 2);}
		} Firmware;
		*/

		struct {
			const string					APIUrl 			= "http://download.look-in.club/firmwares/";
			const uint16_t					ServerPort		= 80;
			const uint8_t					MaxAttempts		= 3;
			const uint16_t					BufferSize		= 2048;
		} OTA;

		struct {
			const string 					BaseURL			= "http://api.look-in.club/v1";

			const string					SyncTime		= BaseURL + "/time";
			const string					Telemetry		= BaseURL + "/telemetry";
			const string					FirmwareCheck	= BaseURL + "/firmwares/check";
			const string					GetACCode		= BaseURL + "/ac/codesets";

			const string					ACtoken			= "2058707b-6fc2-4ff2-b40d-12bcf93c58c7";
		} ServerUrls;

		struct WiFi_t {
			const uint32_t					IPCountdown 	= 90000;
			const uint8_t					SavedAPCount	= 8;
			const string 					DefaultIP		= "192.168.4.1";

			static constexpr uint16_t		UPDPort			= 61201;
			static constexpr uint8_t		UDPHoldPortsMax	= 8;
			const string					UDPPacketPrefix	= "LOOKin:";


			static constexpr uint16_t		MDNSServicePort	= 63091;

			uint32_t						BatteryUptime	= 120* 1000; 	// first time WiFi time to work, ms
			uint32_t						KeepWiFiTime	= 60 * 1000;	// Keep wifi after /network/KeepWiFi command executing, ms

			uint32_t						STAModeInterval = 90; 	// Interval to check if any existed clients nearby, sec
			uint32_t						STAModeReconnect= 60;	// Interval to check if any existed clients nearby after loosing connection, sec

			string							APSSID			= "";
			string							APPassword		= "";

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
			const string					DeviceNamePrefix		= "LOOKin_";
			const string					SecretCodeServiceUUID   = "997d2872-b4e2-414d-b4be-985263a70411";
        	const string 					SecretCodeUUID    		= "1d9fe7b3-5633-4b98-8af4-1bbf2d7c50ea";

        	const int8_t					RSSILimit				= -100; // Ниже этого значения secure endpoints не работают

        	const esp_power_level_t			PublicModePower			= ESP_PWR_LVL_P6;
        	const esp_power_level_t			PrivateModePower		= ESP_PWR_LVL_P6;//ESP_PWR_LVL_P3 и ESP_PWR_LVL_N0; - не работает на части мобильных телефонов?
		} Bluetooth;

		struct RemoteControl_t {
			const string 					Server					= "mqtts://mqtt.look-in.club:8883";
			const string 					ServerUnsecure			= "mqtt://mqtt.look-in.club:1883";
			const uint8_t					MaxConnectionTries		= 3;

			const uint8_t					DefaultQOS				= 2;
			const uint8_t					DefaultRetain			= 0;

			const string					DeviceTopicPrefix		= "/devices/";
		} RemoteControl;

		struct LocalMQTT_t {
			const uint8_t					MaxConnectionTries		= 3;

			const uint8_t					DefaultQOS				= 2;
			const uint8_t					DefaultRetain			= 0;

			const string					TopicPrefix				= "/LOOKin/";
		} LocalMQTT;


		struct WebServer_t {
			const uint16_t					MaxQueryBodyLength		= 4096;
		};

		struct Wireless_t {
			map<uint8_t, pair<uint16_t,uint16_t>> AliveIntervals = {
				{ 0x8, { 294, 6 }}
			};

			const uint8_t					IntervalID 		= 0x8;
		} Wireless;

		struct {
			const uint8_t 					MaxIRItemSignals= 16;
			const uint32_t					StatusSaveDelay	= 10000000; // 10 seconds

		} Data;

		struct {
			const uint8_t 					SystemLogSize	= 16;
			const uint8_t 					EventsLogSize	= 32;
		} Log;

		struct {
			const uint16_t					Interval 		= 1000;

			const uint32_t					MQTTInterval		= 90*1000; 		// 90 секунд
			const uint32_t					ServerPingInterval	= 30*60*1000; 	// 30 минут
			const uint32_t					RouterPingInterval 	= 10*60*1000;	// 10 минут

			struct {
				const uint16_t				Inverval 		= 4000;
				const uint8_t 				OverheatTemp	= 90;
				const uint8_t				ChilledTemp		= 77;
			} OverHeat;

			struct {
				const uint8_t				ActionsDelay	= 5;
				const uint8_t				DefaultValue	= 0xFF;
			} NetworkMap;
		} Pooling;

		struct {
			const uint16_t					ActionsDelay	= 2000;

			const uint16_t					APSuccessDelay	= 2500;
			const uint16_t					STASuccessADelay= 5000;

			const uint8_t					AttemptsToReset	= 10;
			const uint8_t					AttemptsToRevert= 20;
		} BootAndRestore;

		struct Devices_t {
			public:
				static constexpr uint8_t	Duo				= 0x02;
				static constexpr uint8_t	Plug 			= 0x03;
				static constexpr uint8_t	Remote			= 0x81;
				static constexpr uint8_t	Motion			= 0x82;

				map<uint8_t,string> Literaly = {
					{ Duo	, "Duo"		},
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

				void ReadDataOrInit();
				void InitFromNVS();
				uint32_t ReverseBytes(uint32_t);
		} eFuse;

		struct HTTPClient_t {
			static constexpr uint8_t		ThreadsMax		= 2;
			static constexpr uint16_t 		QueueSize		= 16;
			static constexpr uint16_t 		SystemQueueSize	= 8;
			static constexpr uint8_t		BlockTicks		= 50;
			static constexpr uint16_t		NetbuffLen		= 512;
			static constexpr uint8_t		NewThreadStep	= 2;
			static constexpr uint16_t		ThreadStackSize = 4096;
			static constexpr uint16_t		SystemThreadStackSize = 4096;
		} HTTPClient;

		struct Memory_t {
			static constexpr uint8_t		Empty8Bit		= 0xFF;
			static constexpr uint16_t		Empty16Bit		= 0xFFFF;
			static constexpr uint32_t		Empty32Bit		= 0xFFFFFFFF;

			static constexpr uint32_t		BlockSize		= 0x1000;
		} Memory;

		struct Storage_t {
			struct Versions_t {
				static constexpr uint32_t 	StartAddress4MB	= 0x92000;
				static constexpr uint32_t 	Size4MB			= 0xA000;

				static constexpr uint32_t 	StartAddress16MB= 0xAA0000;
				static constexpr uint32_t 	Size16MB		= 0x40000;


				static constexpr uint8_t 	VersionMaxSize	= 10;
			} Versions;

			struct Data_t {
				static constexpr uint32_t 	StartAddress4MB	= 0x9C000;
				static constexpr uint32_t 	Size4MB			= 0x84000;

				static constexpr uint32_t 	StartAddress16MB= 0xAE0000;
				static constexpr uint32_t 	Size16MB		= 0x210000;

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
				static constexpr uint32_t	Start4MB		= 0x32000;
				static constexpr uint32_t	Size4MB			= 0x60000;
				static constexpr uint32_t	ItemSize4MB		= 0x600;	// 1536 байт
				static constexpr uint16_t	Count4MB		= 256;

				static constexpr uint32_t	Start16MB		= 0x8A0000;
				static constexpr uint32_t	Size16MB		= 0x200000;
				static constexpr uint32_t	ItemSize16MB	= 0x600;	// 1536 байт
				static constexpr uint16_t	Count16MB		= 1024;

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

		struct {
			struct {
				const uint16_t				Threshold 		= 9500; 	// Максимальное значени интервала, которое принимается в обработке
				const uint16_t				SignalEndingLen	= 45000;	// Задержка, добавляемая в конец сигнала
				const uint16_t				SignalPauseMax	= 65000;
				const uint32_t				DetectionDelay	= 250000; 	// Временной интервал в течении которого считается, что сигнал 1 (в микросекундах)
				const uint8_t				MinSignalLen	= 10;		// Минимальная длинна сигнала, которая может быть распознана
				const uint16_t				ValueThreshold	= 220;		// Если есть сигнал в посылке с меньшей длительностью - значит засветка
			} IR;

			struct {
				const uint8_t				QueueSize		= 50; 		// Размер очереди сенсора
				const uint16_t				TaskDelay		= 50;		// Задержка в запуске процесса проверки значений в мс
			} TouchSensor;
		} SensorsConfig;

		struct {
			struct {
				const uint64_t				ProntoHexBlockedDelayU = 3*1000000;

				const uint8_t				SendTaskQueueCount1Gen	= 8;
				const uint8_t				SendTaskQueueCount2Gen	= 16;

				const uint16_t				SendTaskPriority		= 2;
				const uint16_t				SendTaskPeakPriority	= 6;
			} IR;

			struct {
				const string				DataPrefix		= "BLE:";
			} BLE;

			struct {
				const uint8_t				QueueSize		= 50; 		// Размер очереди сенсора
				const uint16_t				TaskDelay		= 50;		// Задержка в запуске процесса проверки значений в мс
			} TouchSensor;
		} CommandsConfig;

		// Commands and sensors data

		struct GPIOData_t {
			struct Switch_t {
				gpio_num_t			GPIO 			= GPIO_NUM_0;
			};

			struct MultiSwitch_t {
				vector<gpio_num_t>	GPIO			= vector<gpio_num_t>();
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

				vector<gpio_num_t>	SenderGPIOs		= vector<gpio_num_t>();
				gpio_num_t			SenderGPIOExt	= GPIO_NUM_0;

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

			struct Touch_t {
				vector<gpio_num_t>	GPIO = vector<gpio_num_t>();
			};

			struct DeviceInfo_t {
				Switch_t		Switch;
				MultiSwitch_t	MultiSwitch;
				Color_t			Color;
				IR_t			IR;
				Motion_t		Motion;
				Temperature_t	Temperature;

				// Device modes indicator
				Indicator_t		Indicator;

				// Device PowerMeters
				PowerMeter_t 	PowerMeter;

				Touch_t			Touch;
			};

			DeviceInfo_t GetCurrent();

			static map<uint8_t, DeviceInfo_t> Devices;

			/* Implementation of this settings see Settings.cpp */

		} GPIOData;

		uint8_t DeviceGeneration = 0;
};

extern Settings_t Settings;

#endif
