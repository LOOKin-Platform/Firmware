#include <map>
using namespace std;

#ifndef SETTINGS_H
#define SETTINGS_H

#define WIFI_AP_NAME		"LOOK.in_" + Device.TypeToString() + "_" + Device.IDToString()
#define WIFI_AP_PASSWORD    Device.IDToString()

class Settings_t {
	public:
		const string 						FirmwareVersion = "0.96";

		struct {
			const string					ServerHost 		= "download.look-in.club";
			const uint16_t					ServerPort		= 80;
			const string					UrlPrefix		= "/firmwares/";
		} OTA;

		struct {
			const string 					ServerHost 		= "api.look-in.club";
			const string					APIUrl			= "/v1/time";
		} TimeSync;

		struct WiFi_t {
			const uint16_t					IPCountdown 	= 10000;
			const uint8_t					SavedAPCount	= 8;
			const string 					DefaultIP		= "192.168.4.1";

			const uint16_t					UPDPort			= 61201;
			const string					UDPPacketPrefix	= "LOOK.in:";

			static constexpr uint16_t		HTTPMaxQueryLen	= 6144;

		} WiFi;

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
				static constexpr uint8_t	Default			= 0x81;

				static constexpr uint8_t	Plug 			= 0x03;
				static constexpr uint8_t	Remote			= 0x81;
				static constexpr uint8_t	Motion			= 0x82;

				map<uint8_t,string> Literaly = {
					{ Plug	, "Plug"	},
					{ Remote, "Remote" 	},
					{ Motion, "Motion" 	}
			};
		} Devices;

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
};

extern Settings_t Settings;

#endif

// Commands and Sensors Pin Map

// Plug
#define SWITCH_PLUG_PIN_NUM				GPIO_NUM_23 // GPIO_NUM_2

#define COLOR_PLUG_TIMER_INDEX      	LEDC_TIMER_0
#define COLOR_PLUG_RED_PIN_NUM      	GPIO_NUM_0
#define COLOR_PLUG_RED_PWMCHANNEL   	LEDC_CHANNEL_0
#define COLOR_PLUG_GREEN_PIN_NUM    	GPIO_NUM_0
#define COLOR_PLUG_GREEN_PWMCHANNEL 	LEDC_CHANNEL_1
#define COLOR_PLUG_BLUE_PIN_NUM     	GPIO_NUM_2
#define COLOR_PLUG_BLUE_PWMCHANNEL  	LEDC_CHANNEL_2
#define COLOR_PLUG_WHITE_PIN_NUM    	GPIO_NUM_0
#define COLOR_PLUG_WHITE_PWMCHANNEL 	LEDC_CHANNEL_3

// Remote
#define IR_REMOTE_RECEIVER_GPIO			GPIO_NUM_22 //GPIO_NUM_4
#define IR_REMOTE_SENDER_GPIO			GPIO_NUM_23
#define IR_REMOTE_DETECTOR_PIN			GPIO_NUM_4

#define COLOR_REMOTE_TIMER_INDEX      	LEDC_TIMER_0
#define COLOR_REMOTE_RED_PIN_NUM      	GPIO_NUM_0
#define COLOR_REMOTE_RED_PWMCHANNEL   	LEDC_CHANNEL_0
#define COLOR_REMOTE_GREEN_PIN_NUM    	GPIO_NUM_0
#define COLOR_REMOTE_GREEN_PWMCHANNEL 	LEDC_CHANNEL_1
#define COLOR_REMOTE_BLUE_PIN_NUM     	GPIO_NUM_21
#define COLOR_REMOTE_BLUE_PWMCHANNEL  	LEDC_CHANNEL_2
#define COLOR_REMOTE_WHITE_PIN_NUM    	GPIO_NUM_0
#define COLOR_REMOTE_WHITE_PWMCHANNEL 	LEDC_CHANNEL_3

// MOTION
#define MOTION_GET_INTERVAL				50
#define MOTION_MOTION_CHANNEL			ADC1_CHANNEL_3

// SCENARIOS GLOBALS
#define SCENARIOS_QUEUE_SIZE        	64
#define SCENARIOS_BLOCK_TICKS       	50
#define SCENARIOS_TASK_STACKSIZE    	8192
#define SCENARIOS_OPERAND_BIT_LEN   	64
#define SCENARIOS_NAME_LENGTH			57
#define SCENARIOS_CACHE_BIT_LEN     	32

#define SCENARIOS_TIME_INTERVAL     	5

#define SCENARIOS_TYPE_EVENT_HEX    	0x00
#define SCENARIOS_TYPE_TIMER_HEX    	0x01
#define SCENARIOS_TYPE_CALENDAR_HEX 	0x02

#define SCENARIOS_TYPE_EMPTY_HEX		0xFF

#define MEMORY_BLOCK_SIZE				0x1000

#define MEMORY_SCENARIOS_START			0x32000
#define MEMORY_SCENARIOS_SIZE			0x60000
#define MEMORY_SCENARIOS_OFFSET			0x600 	// 1536 байт
#define MEMORY_SCENARIOS_MAX_COUNT		256

#define MEMORY_SCENARIO_ID				0x0
#define MEMORY_SCENARIO_TYPE			0x4
#define MEMORY_SCENARIO_OPERAND			0x6
#define MEMORY_SCENARIO_NAME			0xE
#define MEMORY_SCENARIO_COMMANDS		0x81

#define MEMORY_COMMAND_SIZE				0xA
#define MEMORY_COMMAND_DEVICEID			0x0
#define MEMORY_COMMAND_COMMANDID		0x4
#define MEMORY_COMMAND_EVENTID			0x5
#define MEMORY_COMMAND_OPERAND			0x6
