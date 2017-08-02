/*
*    Automation Rules
*    API /Automation
*
*/

#ifndef AUTOMATION_H
#define AUTOMATION_H

using namespace std;

#include <string>
#include <vector>

#include "API.h"
#include "Scenarios.h"
#include "JSON.h"

#include "WiFi/WiFi.h"
#include "NVS/NVS.h"

#include "Converter.h"

#define  NVSScenariosArray "Scenarios"

struct ScenarioCacheItem_t {
  bool      IsLinked      = false;
  uint32_t  ScenarioID    = 0;
  uint8_t   ScenarioType  = 0;
  uint64_t  Operand       = 0;
  uint8_t   ArrayID       = 0;
};

class Automation_t : public API {
  public:
    Automation_t();
    void Init();

    uint32_t  CurrentVersion();

    void        SensorChanged(uint8_t SensorID, uint8_t EventCode);
    static void TimeChangedTask(void *);

    uint8_t   CacheGetArrayID(string ScenarioID);
    uint8_t   CacheGetArrayID(uint32_t ScenarioID);

    string    MemoryGetScenarioByID(uint32_t);

    void      HandleHTTPRequest(WebServerResponse_t* &, QueryType Type, vector<string>, map<string,string>, string = "");
  private:
    TaskHandle_t TimeChangedHandle;

    vector<ScenarioCacheItem_t> ScenariosCache;
    map<string, uint32_t>       VersionMap;

    bool  AddScenario             (string);
    bool  AddScenario             (Scenario_t* &);
    void  AddScenarioCacheItem    (Scenario_t* &, int);

    void  RemoveScenario          (uint32_t);
    void  RemoveScenarioCacheItem (uint32_t);

    void  Debug(ScenarioCacheItem_t);
    void  Debug(Scenario_t* &);
};

#endif //AUTOMATION_H
