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

#include "Query.h"
#include "WebServer.h"

#define  NVSLogArea   "Log"
#define  NVSLogArray  "SystemLog"

/**
 * @brief Interface to logging functions
 */
class Log {
	public:
		struct Item {
			uint16_t  Code  = 0;
			uint32_t  Data  = 0;
			uint32_t  Time  = 0;
		};

		enum ItemType { SYSTEM, ERROR, INFO };

		Log();

		static void         Add(uint16_t Code, uint32_t Data = 0);

		static vector<Item> GetSystemLog();
		static uint8_t      GetSystemLogCount(NVS *Memory = nullptr);
		static Item         GetSystemLogItem(uint8_t Index, NVS *Memory = nullptr);
		static string       GetSystemLogJSONItem(uint8_t Index, NVS *MemoryLink = nullptr);
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
				enum MODE { NONE, CONST, BLINKING, STROBE };

				static void Display(uint16_t);
				static void Execute(uint8_t Red, uint8_t Green, uint8_t Blue, MODE Blinking, uint8_t Duration = 0);
			private:
				static bool		IsInited;
				static constexpr float Brightness = 0.03;
				static uint32_t	tDuration;
				static uint32_t	tExpired;
				static MODE		tBlinking;

				static uint8_t	tRed, tGreen, tBlue;
			    static ISR::HardwareTimer IndicatorTimer;

			    static void IndicatorCallback(void *);
		} Indicator;

		static void         HandleHTTPRequest(WebServer_t::Response &, QueryType, vector<string>, map<string,string>);

	private:

		static vector<Log::Item> Items;
		static string       Serialize(Log::Item Item);
		static Log::Item    Deserialize(string JSONString);

	public:
		 // 1XXX - информация, 0XXX - ошибки
		class Events {
			public:
				enum System {
					// Cистемные события 0x0000..0x00FF - сохраняются в NVS
					DeviceOn 		= 0x0001,
					DeviceStarted 	= 0x0002,
					DeviceRollback	= 0x00f0,
					OTASucced		= 0x0040,
					OTAVerifyFailed	= 0x0041,
					OTAFailed		= 0x1142,

					// Неинвазивные системные события 0xX100...0xX1FF
					DeviceOverheat	= 0x1105,
					DeviceCooled	= 0x1106,
					TimeSynced		= 0x1130,
					OTAStarted		= 0x1140
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
				};

				// Bluetooth события 0xX300...0xX3FF

				enum Sensors {
					// События сенсоров 0xX400...0xX4FF
					IRReceived		= 0x1487
				};

				enum Commands {
					// События команд 0xX500...0xX5FF
					IRFailed 		= 0x0507,
					IRExecuted		= 0x1507
				};

				// События автоматизации 0xX600...0xX6FF

				// События Storage 0xX700...0xX7FF
			} ;
};

#define TIMER_ALARM					100000 // 1/10 секунды
#define BLINKING_DIVIDER			7 // моргание каждые 0.7 секунды
#define	STROBE_DIVIDER				1 // стробоскоп - каждые 0.1 секунды

#endif
