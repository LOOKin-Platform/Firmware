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

    static void         HandleHTTPRequest(WebServer_t::Response* &, QueryType, vector<string>, map<string,string>);

  private:
    static vector<Log::Item> Items;
    static string       Serialize(Log::Item Item);
    static Log::Item    Deserialize(string JSONString);
};

// Cистемные события 0x0000..0x00FF - сохраняются в NVS
// События с кодом, 1XXX - информация, 0XXX - ошибки
#define LOG_DEVICE_ON               0x0001
#define LOG_DEVICE_STARTED          0x0002

#define LOG_DEVICE_ROLLBACK         0x00F0

// Неинвазивные системные события 0xX100...0xX1FF

#define LOG_DEVICE_OVERHEAT         0x1105
#define LOG_DEVICE_COOLLED          0x1106

#define LOG_TIME_SYNCED             0x1130

// Wi-FI и Bluetooth события 0xX200...0xX2FF

#define LOG_WIFI_AP_START           0x1201
#define LOG_WIFI_AP_STOP            0x1202
#define LOG_WIFI_STA_CONNECTED      0x1203
#define LOG_WIFI_STA_DISCONNECTED   0x1204
#define LOG_WIFI_STA_GOT_IP         0x1205
#define LOG_WIFI_STA_UNDEFINED_IP   0x0206

// Bluetooth события 0xX300...0xX3FF

// События сенсоров 0xX400...0xX4FF

// События команд 0xX500...0xX5FF

// События сценариев 0xX600...0xX6FF

#endif
