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

#include "Time/Time.h"
#include "FreeRTOS/Timer.h"
#include "HTTPClient/HTTPClient.h"

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
		char        Name[64];
    Data_t      *Data;

    vector<ScenesCommandItem_t> Commands;

    Scenario_t(uint8_t TypeHex);

    string  GetDataHexString();
    void    SetData(uint64_t);
    void    SetData(string);
    bool    Empty();

    /************************************/
    /*    Static methods to             */
    /*    Execute Scenario commands     */
    /************************************/

    static void         ExecuteScenario(uint32_t ScenarioID);
    static void         ExecuteCommandsTask(void *);

    static void         LoadScenarios();
    static string       LoadScenario(uint8_t Index);
    static bool         SaveScenario(Scenario_t *);
    static void         RemoveScenario(uint8_t Index);

    static string       SerializeScene(Scenario_t *);
    static Scenario_t*  DeserializeScene(string);

    template <size_t ResultSize>  static bitset<ResultSize> Range(bitset<SCENARIOS_OPERAND_BIT_LEN>, size_t Start, size_t Length);
    template <size_t SrcSize>     static void AddRangeTo(bitset<SCENARIOS_OPERAND_BIT_LEN> &, bitset<SrcSize>, size_t Position);
    static bitset<8> Bitset4To8(bitset<4>);

    // HTTP Callbacks
    static void ReadFinished(char[]);
    static void Aborted(char[]);

  private:
    static QueueHandle_t  Queue;
    static uint8_t        ThreadsCounter;
};

class Data_t {
  public:
    virtual bool      IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  { return false; };
    virtual void      SetData(bitset<SCENARIOS_OPERAND_BIT_LEN>) {};
    virtual string    ToString() { return ""; };

    virtual bool      IsCommandNeedToExecute(ScenesCommandItem_t &) {return true;};
    virtual void      ExecuteCommands(uint32_t ScenarioID)  {};
    virtual bool      SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t EventCode) { return false; };
    virtual bool      TimeUpdatedIsTriggered() { return false; };
};

class EventData_t : public Data_t {
  public:
		uint32_t    DeviceID          = 0;
		uint8_t     SensorIdentifier  = 0;
		uint8_t     EventCode         = 0;

    bool        IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  override;
    void        SetData(bitset<SCENARIOS_OPERAND_BIT_LEN>) override;
    string      ToString() override;

    void        ExecuteCommands(uint32_t ScenarioID)  override;
    bool        SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t EventCode) override;
};

class TimerData_t : public Data_t {
  public:
    struct TimerDataStruct {
      uint32_t ScenarioID = 0;
      uint16_t TimerDelay = 0;
    };

  	uint32_t    DeviceID          = 0;
  	uint8_t     SensorIdentifier  = 0;
  	uint8_t     EventCode         = 0;
    uint16_t    TimerDelay        = 0; // максимально - 12 бит или 4096 секунд

    bool        IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  override;
    void        SetData(bitset<SCENARIOS_OPERAND_BIT_LEN>) override;
    string      ToString() override;

    void        ExecuteCommands(uint32_t ScenarioID) override;
    bool        SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t EventCode) override;

    static void TimerCallback(Timer_t *pTimer);
};

class CalendarData_t : public Data_t {
  public:
    bool        IsScheduled      = false;
    DateTime_t  DateTime;
    bitset<8>   ScheduledDays;

    bool        IsLinked(uint32_t, const vector<ScenesCommandItem_t>& = vector<ScenesCommandItem_t>())  override;
    void        SetData(bitset<SCENARIOS_OPERAND_BIT_LEN>) override;
    string      ToString() override;

    bool        IsCommandNeedToExecute(ScenesCommandItem_t &) override;
    void        ExecuteCommands(uint32_t ScenarioID) override;
    bool        TimeUpdatedIsTriggered() override;
};


#endif /* SCENARIOUS_H */
