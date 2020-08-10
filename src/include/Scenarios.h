/*
*    Scanarios
*    Subpart of API /Automation
*
*/

#ifndef SCENARIOUS_H
#define SCENARIOUS_H

using namespace std;

#include "Settings.h"

#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <algorithm>

#include <esp_log.h>

#include "DateTime.h"
#include "FreeRTOSWrapper.h"
#include "JSON.h"
#include "HTTPClient.h"

#include "Memory.h"
#include "WebServer.h"
#include "Converter.h"

struct ScenesCommandItem_t {
	uint32_t  DeviceID    = 0;
	uint8_t   CommandID   = 0;
	uint8_t   EventCode   = 0;
	uint32_t  Operand     = 0;
};

class Data_t;

class Scenario_t {
	public:
		uint8_t     Type;
		uint32_t    ID;
		uint16_t	Name[Settings.Scenarios.NameLength] = { 0 };
		Data_t      *Data;

		vector<ScenesCommandItem_t> Commands;

		Scenario_t(uint8_t TypeHex = Settings.Scenarios.Types.EmptyHex);
		~Scenario_t();

		void				SetType			(uint8_t TypeHex);
		uint64_t  			GetDataUint64	();
		string  			GetDataHexString();
		void    			SetData			(uint64_t);
		void    			SetData			(string);

		bool    			IsEmpty();

		static void			ExecuteScenario(uint32_t ScenarioID);
		static void			ExecuteCommandsTask(void *);

		static void			LoadScenarios();
		static void			LoadScenario(Scenario_t &, uint32_t ScenarioID);
		static void			LoadScenarioByAddress(Scenario_t &, uint32_t Address);

		static bool			SaveScenario	(Scenario_t);
		static void			RemoveScenario	(uint32_t ScenarioID);

		static string		SerializeScene	(uint32_t ScanarioID);
		static string		SerializeScene	(Scenario_t);
		static Scenario_t	DeserializeScene(const char*);

		template <size_t ResultSize>  static bitset<ResultSize> Range(bitset<Settings_t::Scenarios_t::OperandBitLength>, size_t Start, size_t Length);
		template <size_t SrcSize>     static void AddRangeTo(bitset<Settings_t::Scenarios_t::OperandBitLength> &, bitset<SrcSize>, size_t Position);

		// HTTP Callbacks
		static void ReadFinished(char[]);
		static void Aborted(char[]);

	private:
		static QueueHandle_t  Queue;
		static uint8_t        ThreadsCounter;
};

class Data_t {
  public:
    virtual bool      	IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  { return false; };
    virtual void      	SetData(bitset<Settings.Scenarios.OperandBitLength>) {};
    virtual uint64_t  	ToUint64() { return 0; };
    virtual string    	ToString() { return ""; };

    virtual bool      	IsCommandNeedToExecute(ScenesCommandItem_t &) {return true;};
    virtual void      	ExecuteCommands(uint32_t ScenarioID)  {};
    virtual bool      	SensorUpdatedIsTriggered(uint8_t SensorID) { return false; };
    virtual bool      	TimeUpdatedIsTriggered() { return false; };

    virtual ~Data_t() { 	ESP_LOGI("DESTRUCTOR", "DATA_t");};
};

class EventData_t : public Data_t {
  public:
	uint32_t			DeviceID          = 0;
	uint8_t				SensorIdentifier  = 0;
    uint8_t				EventCode         = 0;
    uint8_t				EventOperand      = 0;

    bool				IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  override;
    void				SetData(bitset<Settings.Scenarios.OperandBitLength>) override;

    uint64_t			ToUint64() override;
    string				ToString() override;

    void				ExecuteCommands(uint32_t ScenarioID)  override;
    bool				SensorUpdatedIsTriggered(uint8_t SensorID) override;

    ~EventData_t() 		{ ESP_LOGI("DESTRUCTOR", "EventDATA_t"); };
};

class TimerData_t : public Data_t {
  public:
    struct TimerDataStruct {
      uint32_t 			ScenarioID = 0;
      uint16_t 			TimerDelay = 0;
    };

  	uint32_t			DeviceID          = 0;
  	uint8_t				SensorIdentifier  = 0;
  	uint8_t				EventCode         = 0;
    uint16_t			TimerDelay        = 0; // максимально - 8 бит или 1280 секунд с шагом в 5 секунд
    uint8_t				EventOperand      = 0;

    bool				IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  override;
    void				SetData(bitset<Settings.Scenarios.OperandBitLength>) override;

    uint64_t			ToUint64() override;
    string				ToString() override;

    void				ExecuteCommands(uint32_t ScenarioID) override;
    bool				SensorUpdatedIsTriggered(uint8_t SensorID) override;

    static void			TimerCallback(FreeRTOS::Timer *pTimer);

    ~TimerData_t() 		{ ESP_LOGI("DESTRUCTOR", "TimerDATA_t"); };
};

class CalendarData_t : public Data_t {
  public:
    bool				IsScheduled = false;
    DateTime_t			DateTime;
    bitset<8>			ScheduledDays;

    bool				IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  override;
    void				SetData(bitset<Settings.Scenarios.OperandBitLength>) override;

    uint64_t			ToUint64() override;
    string				ToString() override;

    bool				IsCommandNeedToExecute(ScenesCommandItem_t &) override;
    void				ExecuteCommands(uint32_t ScenarioID) override;
    bool				TimeUpdatedIsTriggered() override;

    ~CalendarData_t() {};
};


#endif /* SCENARIOUS_H */
