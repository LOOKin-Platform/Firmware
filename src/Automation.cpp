/*
*    Automation Rules
*    API /Automation
*
*/
#include "Automation.h"
#include "Globals.h"

static char tag[] = "Automation";
static string NVSAutomationArea = "Automation";

Automation_t::Automation_t() {
  ScenariosCache  = vector<ScenarioCacheItem_t>();
  VersionMap      = map<string, uint32_t>();
}

void Automation_t::Init() {
  ESP_LOGD(tag, "Init");

  NVS *Memory = new NVS(NVSAutomationArea);

  // Create scenarious cache from scenarios saved in NVS
  uint8_t ArrayCount = Memory->ArrayCount(NVSScenariosArray);

  for (int i=0; i < ArrayCount; i++) {
    Scenario_t *Scenario = Scenario_t::DeserializeScene(Memory->StringArrayGet(NVSScenariosArray, i));
    AddScenarioCacheItem(Scenario, i);
    delete Scenario;
  }

  ESP_LOGI(tag, "Loaded %i scenarios", ArrayCount);
  delete Memory;

  TimeChangedHandle = FreeRTOS::StartTask(TimeChangedTask, "TimeChangedTask", NULL, 4096);
}

uint32_t Automation_t::CurrentVersion() {
  if (WiFi_t::getMode() == WIFI_MODE_AP_STR) {
    string ApMAC = WiFi_t::getApMac();
    if (VersionMap.count(ApMAC) > 0) {
      return VersionMap[ApMAC];
    }
  }
  return 0;
}

void Automation_t::SensorChanged(uint8_t SensorID, uint8_t EventCode) {
  ESP_LOGI(tag, "Sensor Changed. ID: %s, Event: %s",
                      Converter::ToHexString(SensorID, 2).c_str(),
                      Converter::ToHexString(EventCode, 2).c_str());

  NVS *Memory = new NVS(NVSAutomationArea);

  for (auto& ScenarioCacheItem : ScenariosCache)
    if (ScenarioCacheItem.IsLinked == true) {
      Scenario_t *Scenario = new Scenario_t(ScenarioCacheItem.ScenarioType);
      Scenario->SetData(ScenarioCacheItem.Operand);

      if (Scenario->Data->SensorUpdatedIsTriggered(SensorID, EventCode)) {
        Scenario_t *ScenarioToExecute = Scenario_t::DeserializeScene(Memory->StringArrayGet(NVSScenariosArray,ScenarioCacheItem.ArrayID));

        if (ScenarioToExecute!=nullptr) {
          ScenarioToExecute->Data->ExecuteCommands(ScenarioToExecute->Commands, ScenarioCacheItem.ScenarioID);
          delete ScenarioToExecute;
        }

      }

      delete Scenario;
    }

  delete Memory;
}

void Automation_t::TimeChangedTask(void *) {
  while (1) {

    while (Time::Unixtime()%SCENARIO_TIME_INTERVAL!=0)
      FreeRTOS::Sleep(1000);

    for (auto& ScenarioCacheItem : Automation->ScenariosCache)
      if (ScenarioCacheItem.IsLinked == true && ScenarioCacheItem.ScenarioType == 0x2) {
        Scenario_t *Scenario = new Scenario_t(ScenarioCacheItem.ScenarioType);
        Scenario->SetData(ScenarioCacheItem.Operand);

        if (Scenario->Data->TimeUpdatedIsTriggered()) {
          NVS *Memory = new NVS(NVSAutomationArea);
          Scenario_t *ScenarioToExecute = Scenario_t::DeserializeScene(Memory->StringArrayGet(NVSScenariosArray,ScenarioCacheItem.ArrayID));
          if (ScenarioToExecute!=nullptr) {
            ScenarioToExecute->Data->ExecuteCommands(ScenarioToExecute->Commands, ScenarioCacheItem.ScenarioID);
            delete ScenarioToExecute;
          }
          delete Memory;
        }
        delete Scenario;
      }

    if (SCENARIO_TIME_INTERVAL > 1)
      FreeRTOS::Sleep(1000);

  }
}

uint8_t Automation_t::CacheGetArrayID(string ScenarioID) {
  return CacheGetArrayID(Converter::UintFromHexString<uint32_t>(ScenarioID));
}

uint8_t Automation_t::CacheGetArrayID(uint32_t ScenarioID) {
  for (auto& Scenario : ScenariosCache)
    if (Scenario.ScenarioID == ScenarioID)
      return Scenario.ArrayID;

  return MAX_NVSARRAY_INDEX+1;
}

string Automation_t::MemoryGetScenarioByID(uint32_t ScenarioID) {
  NVS *Memory = new NVS(NVSAutomationArea);
  string Result = Memory->StringArrayGet(NVSScenariosArray, CacheGetArrayID(ScenarioID));
  delete Memory;
  return Result;
}


void Automation_t::HandleHTTPRequest(WebServerResponse_t* &Result, QueryType Type, vector<string> URLParts,
                                                  map<string,string> Params, string RequestBody) {
  // обработка GET запроса - получение данных
  if (Type == QueryType::GET) {
    // Запрос корневого JSON /scenes
    if (URLParts.size() == 0) {

      JSON_t *JSON = new JSON_t();

      JSON->SetParam("Version", Converter::ToHexString(CurrentVersion(), 8));

      vector<map<string,string>> tmpVector = vector<map<string,string>>();
      for (map<string, uint32_t>::iterator it = VersionMap.begin(); it != VersionMap.end(); ++it) {
        tmpVector.push_back({
          { "BSSID"  , it->first},
          { "Version", Converter::ToHexString(it->second, 8)}
        });
      }

      JSON->SetMapsArray("VersionMap", tmpVector);

      vector<string> SceneIDs;
      for (auto& Scenario : ScenariosCache)
        SceneIDs.push_back(Converter::ToHexString(Scenario.ScenarioID, 8));

      JSON->SetVector("Scenarios", SceneIDs);

      Result->Body = JSON->ToString();
      delete JSON;
    }

    // Запрос конкретного параметра или команды секции API
    if (URLParts.size() == 1) {
      if (URLParts[0] == "version")   Result->Body = Converter::ToHexString(CurrentVersion(), 8);
      if (URLParts[0] == "scenarios") {
        JSON_t *JSON = new JSON_t();

        for (auto& Scenario : ScenariosCache)
          JSON->AddToVector(Converter::ToHexString(Scenario.ScenarioID, 8));

        Result->Body = JSON->ToString(true);
        delete JSON;
      }
    }

    if (URLParts.size() == 2) {
      if (URLParts[0] == "scenarios") {
        NVS *Memory = new NVS(NVSAutomationArea);
        Result->Body = Memory->StringArrayGet(NVSScenariosArray,CacheGetArrayID(URLParts[1]));
        delete Memory;
      }
    }
  }

  // POST запрос - сохранение и изменение данных
  if (Type == QueryType::POST) {
    if (URLParts.size() == 0) {

      Scenario_t *Scenario = Scenario_t::DeserializeScene(RequestBody);

      bool IsFound = false;
      if (Scenario != NULL) {
        for (int i=0; i < ScenariosCache.size(); i++)
          if (ScenariosCache[i].ScenarioID == Scenario->ID) {
            IsFound = true;
            break;
          }

        if (!IsFound) {
          if (AddScenario(Scenario))
            Result->Body = "{\"success\" : \"true\"}";
          else {
            Result->ResponseCode = WebServerResponse_t::CODE::ERROR;
            Result->Body = "{\"success\" : \"false\", \"message\" : \"Error while scenario insertion\"}";
          }
        }
        else {
          Result->ResponseCode = WebServerResponse_t::CODE::INVALID;
          Result->Body = "{\"success\" : \"false\", \"message\" : \"Scenario already set\"}";
        }
      }

      delete Scenario;
    }
  }

  // DELETE запрос - удаление сценариев
  if (Type == QueryType::DELETE) {
    if (URLParts.size() == 2) {
      if (URLParts[0] == "scenarios") {
        uint32_t ScenarioID = Converter::UintFromHexString<uint32_t>(URLParts[1]);
        RemoveScenario(ScenarioID);
        RemoveScenarioCacheItem(ScenarioID);

        Result->ResponseCode = WebServerResponse_t::CODE::OK;
        Result->Body = "{\"success\" : \"true\"}";
      }
    }
  }
}

bool Automation_t::AddScenario(Scenario_t * &Scenario) {
  bool Result = false;
  NVS *Memory = new NVS(NVSAutomationArea);
  uint8_t Index = Memory->StringArrayAdd(NVSScenariosArray, Scenario_t::SerializeScene(Scenario));
  Memory->Commit();
  if (Index != MAX_NVSARRAY_INDEX+1) {
    AddScenarioCacheItem(Scenario, Index);
    Result = true;
  }

  delete Memory;
  return Result;
}

void Automation_t::AddScenarioCacheItem(Scenario_t* &Scenario, int ArrayIndex) {
  ScenarioCacheItem_t NewCacheItem;

  NewCacheItem.IsLinked       = Scenario->Data->IsLinked(Device->ID, Scenario->Commands);
  NewCacheItem.ScenarioID     = Scenario->ID;
  NewCacheItem.ScenarioType   = Scenario->Type;
  NewCacheItem.Operand        = Converter::UintFromHexString<uint64_t>(Scenario->Data->ToString());
  NewCacheItem.ArrayID        = ArrayIndex;

  ScenariosCache.push_back(NewCacheItem);
}

void Automation_t::RemoveScenario(uint32_t ScenarioID) {
  NVS *Memory = new NVS(NVSAutomationArea);
  Memory->StringArrayRemove(NVSScenariosArray, +CacheGetArrayID(ScenarioID));
  Memory->Commit();
  delete Memory;
}

void Automation_t::RemoveScenarioCacheItem(uint32_t ScenarioID) {
  for (int i=0; i < ScenariosCache.size(); i++)
    if (ScenariosCache[i].ScenarioID == ScenarioID) {
      ScenariosCache.erase(ScenariosCache.begin() + i);
      return;
    }
}


void Automation_t::Debug(ScenarioCacheItem_t Item) {
  ESP_LOGI(tag, "IsLinked     = %u"   , Item.IsLinked);
  ESP_LOGI(tag, "ScenarioID   = %s"   , Converter::ToHexString(Item.ScenarioID, 2).c_str());
  ESP_LOGI(tag, "ScenarioType = %s"   , Converter::ToHexString(Item.ScenarioType,2).c_str());
  ESP_LOGI(tag, "Operand      = %s"   , Converter::ToHexString(Item.Operand, 16).c_str());
  ESP_LOGI(tag, "ArrayID      = %u"   , Item.ArrayID);
}

void Automation_t::Debug(Scenario_t* &Scene) {
  ESP_LOGI(tag, "Scene Type = %s"   , Converter::ToHexString(Scene->Type,2).c_str());
  ESP_LOGI(tag, "Scene ID   = %s"   , Converter::ToHexString(Scene->ID,8).c_str());
  ESP_LOGI(tag, "Scene Name = %s"   , Scene->Name);

  /*
  if (Scene->Type == SCENARIO_TYPE_EVENT_HEX) {
    ESP_LOGI(tag, "DeviceID          = %s", Converter::ToHexString(Scene->EventData->DeviceID,8).c_str());
  	ESP_LOGI(tag, "SensorIdentifier  = %s", Converter::ToHexString(Scene->EventData->SensorIdentifier,2).c_str());
  	ESP_LOGI(tag, "EventCode         = %s", Converter::ToHexString(Scene->EventData->EventCode,2).c_str());
  }

  if (Scene->Type == SCENARIO_TYPE_TIMER_HEX) {
    ESP_LOGI(tag, "DeviceID          = %s", Converter::ToHexString(Scene->TimerData->DeviceID,8).c_str());
  	ESP_LOGI(tag, "SensorIdentifier  = %s", Converter::ToHexString(Scene->TimerData->SensorIdentifier,2).c_str());
    ESP_LOGI(tag, "EventCode         = %s", Converter::ToHexString(Scene->TimerData->EventCode,2).c_str());
    ESP_LOGI(tag, "Timer Delay (sec) = %u", Scene->TimerData->TimerDelay);
  }
  */

  for (int i=0; i < Scene->Commands.size(); i++) {
    ESP_LOGI(tag, "Command[%u] DeviceID   = %s", i, Converter::ToHexString(Scene->Commands[i].DeviceID,8).c_str());
    ESP_LOGI(tag, "Command[%u] Command    = %s", i, Converter::ToHexString(Scene->Commands[i].CommandID,2).c_str());
    ESP_LOGI(tag, "Command[%u] Event      = %s", i, Converter::ToHexString(Scene->Commands[i].EventCode,2).c_str());
  }

}
