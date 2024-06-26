/*
*    Log.h
*    Class to handle API /Log
*
*/

#ifndef LOG_H
#define LOG_H

using namespace std;

#include <string>
#include <esp_log.h>

#include "JSON.h"
#include "Memory.h"
#include "DateTime.h"
#include "ISR.h"
#include "HardwareIO.h"
#include "ws2812.h"
#include "esp_timer.h"

#include "Query.h"
#include "WebServer.h"

#define  NVSLogArea   "Log"
#define  NVSLogArray  "SystemLog"
#define  NVSBrightness "Brightness"

/**
 * @brief Interface to logging functions
 */
class Log {
	public:
		union __attribute__((__packed__)) Item {
			struct { uint16_t Code : 16, Data : 16; uint32_t Time : 32; };
			uint64_t Uint64Data = 0;
		};

		enum ItemType { SYSTEM, ERROR, INFO };

		Log();

		static void         Add(uint16_t Code, uint32_t Data = 0);

		static vector<Item> GetSystemLog();
		static string       GetSystemLogJSON();

		static vector<Item> GetEventsLog();
		static uint8_t      GetEventsLogCount();
		static Item         GetEventsLogItem(uint8_t Index);
		static string       GetEventsLogJSONItem(uint8_t Index);
		static string       GetEventsLogJSON();

		static ItemType     GetItemType(uint16_t Code);

		static void         CorrectTime();
		static bool         VerifyLastBoot();

		class Indicator_t {
			public:
				enum 						MODE { NONE, CONST, BLINKING, STROBE };

				static void 				Display(uint16_t);
				static void 				Execute(uint8_t Red, uint8_t Green, uint8_t Blue, MODE Blinking, uint16_t Duration = 0);

			    static uint8_t 				GetBrightness();
			    static void 				SetBrightness(uint8_t);
			private:
				static bool					IsInited;
				static float 				CurrentBrightness;
				static constexpr float 		BrightnessDelimeter = 1000;

				static uint32_t				tDuration;
				static uint32_t				tExpired;
				static MODE					tBlinking;

				static uint8_t				tRed, tGreen, tBlue;
			    static esp_timer_handle_t 	IndicatorTimer;

			    static void IndicatorCallback(void *);
		} Indicator;

		static void         HandleHTTPRequest(WebServer_t::Response &, Query_t &Query);

	private:

		static vector<Log::Item> Items;
		static string       Serialize(Log::Item Item);
		static Log::Item    Deserialize(string JSONString);

		static WS2812		*ws2812;
	public:
		 // 1XXX - информация, 0XXX - ошибки
		class Events {
			public:
				enum System {
					// Cистемные события 0x0000..0x00FF - сохраняются в NVS
					DeviceOn 		= 0x0001,
					DeviceStarted 	= 0x0002,
					PowerManageOn	= 0x0003,
					PowerManageOff	= 0x0004,
					DeviceRollback	= 0x00f0,
					HardResetIntied	= 0x00f2,
					OTASucceed		= 0x0040,
					OTAVerifyFailed	= 0x0041,

					FirstDeviceInit	= 0x00f1,

					// Неинвазивные системные события 0xX100...0xX1FF
					DeviceOverheat	= 0x1105,
					DeviceCooled	= 0x1106,
					TimeSynced		= 0x1130,
					OTAStarted		= 0x1140,
					OTAFailed		= 0x1142
				};

				enum WiFi {
					// Wi-FI события 0xX200...0xX2FF
					APStart			= 0x1201,
					APStop			= 0x1202,
					STAConnecting	= 0x1203,
					STAConnected	= 0x1204,
					STADisconnected	= 0x1205,
					STAUndefinedIP	= 0x0206,
					STAGotIP		= 0x1206,
					STAStarted		= 0x1207,
					STAUnreachable	= 0x1208
				};

				// Bluetooth события 0xX300...0xX3FF

				enum Sensors {
					// События сенсоров 0xX400...0xX4FF
					IRReceived		= 0x1487
				};

				enum Commands {
					// События команд 0xX500...0xX5FF
					IRFailed 		= 0x0507,
					IRExecuted		= 0x1507,

					BLEFailed		= 0x0508,
					BLEExecuted		= 0x1508
				};

				enum Misc {
					// События Ho 0xXF00...0xXFFF
					HomeKitIdentify = 0x1F00,
				};

				// События автоматизации 0xX600...0xX6FF

				// События Storage 0xX700...0xX7FF
			} ;
};

#define TIMER_ALARM					100000 // 1/10 секунды
#define BLINKING_DIVIDER			7 // моргание каждые 0.7 секунды
#define	STROBE_DIVIDER				1 // стробоскоп - каждые 0.1 секунды

#endif
