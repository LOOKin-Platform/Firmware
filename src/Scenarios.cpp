/*
*    Scanarios
*    Subpart of API /Automation
*
*/
#include "Globals.h"
#include "Scenarios.h"

static char tag[] = "Scenarios";
static string NVSAutomationArea = "Automation";

/************************************/
/*         Scenario class           */
/************************************/

QueueHandle_t Scenario_t::Queue = FreeRTOS::Queue::Create(SCENARIOS_QUEUE_SIZE, sizeof( uint32_t ));
uint8_t       Scenario_t::ThreadsCounter = 0;

Scenario_t::Scenario_t(uint8_t TypeHex) {
  this->Type                  = TypeHex;
  this->Commands              = vector<ScenesCommandItem_t>();

  switch (Type) {
    case SCENARIOS_TYPE_EVENT_HEX     : this->Data = new EventData_t();        break;
    case SCENARIOS_TYPE_TIMER_HEX     : this->Data = new TimerData_t();        break;
    case SCENARIOS_TYPE_CALENDAR_HEX  : this->Data = new CalendarData_t();     break;
  }
}

string Scenario_t::GetDataHexString() {
  return Data->ToString();
}

void Scenario_t::SetData(uint64_t Operand) {
  bitset<SCENARIOS_OPERAND_BIT_LEN> DataBitset(Operand);
  Data->SetData(DataBitset);
}

void Scenario_t::SetData(string HexString) {

  if (HexString.length() > 16) HexString.substr(0, 16);
  if (HexString.length() < 16) while (HexString.length() < 16) HexString += "0";

  bitset<SCENARIOS_OPERAND_BIT_LEN> DataBitset(Converter::UintFromHexString<uint64_t>(HexString));
  Data->SetData(DataBitset);
}

bool Scenario_t::Empty() {
  return (ID == 0 && Type == 0x00 && string(Name).empty()) ? true : false;
}

void Scenario_t::ExecuteScenario(uint32_t ScenarioID) {
  FreeRTOS::Queue::SendToBack(Scenario_t::Queue, &ScenarioID, (TickType_t) SCENARIOS_BLOCK_TICKS );

  if (ThreadsCounter <= 0 && FreeRTOS::Queue::Count(Scenario_t::Queue)) {
    ThreadsCounter = 1;
    FreeRTOS::StartTask(ExecuteCommandsTask, "ExecuteCommandsTask", ( void * )(uint32_t)ThreadsCounter, SCENARIOS_TASK_STACKSIZE);
  }
}

void Scenario_t::ExecuteCommandsTask(void *TaskData) {
  ESP_LOGD(tag, "Scenario executed task %u created", (uint32_t) TaskData);

  uint32_t ScenarioID = 0;

  NVS *Memory = new NVS(NVSAutomationArea);

  if (Queue != 0)
    while (FreeRTOS::Queue::Receive(Scenario_t::Queue, &ScenarioID, (TickType_t) SCENARIOS_TASK_STACKSIZE)) {

      Scenario_t *Scenario = DeserializeScene(Memory->StringArrayGet(NVSScenariosArray, Automation->CacheGetArrayID(ScenarioID)));

      for (ScenesCommandItem_t Command : Scenario->Commands)
        if (Scenario->Data->IsCommandNeedToExecute(Command)) {
          if (Command.DeviceID == Device->ID) {
            // выполнить локальную команду
            Command_t *CommandToExecute = Command_t::GetCommandByID(Command.CommandID);
            if (CommandToExecute!=nullptr)
              CommandToExecute->Execute(Command.EventCode, Command.Operand);
          }
          else {
            // отправить команду по HTTP
            NetworkDevice_t NetworkDevice = Network->GetNetworkDeviceByID(Command.DeviceID);

            if (!NetworkDevice.IP.empty() && NetworkDevice.IsActive) {
              string URL = "/commands?command=" + Converter::ToHexString(Command.CommandID,2) +
                                     "&action=" + Converter::ToHexString(Command.EventCode,2) +
                                    "&operand=" + Converter::ToHexString(Command.Operand, 8);

              HTTPClient::Query("", 80, URL, QueryType::GET, NetworkDevice.IP, false, NULL, NULL, &ReadFinished, &Aborted);
            }
          }
        }

      delete Scenario;
    }

  delete Memory;

  ESP_LOGD(tag, "Task %u removed", (uint32_t)TaskData);
  Scenario_t::ThreadsCounter--;
  FreeRTOS::DeleteTask();
}

void Scenario_t::LoadScenarios() {
  NVS *Memory = new NVS(NVSAutomationArea);

  uint8_t ArrayCount = Memory->ArrayCount(NVSScenariosArray);

  for (int i=0; i < ArrayCount; i++) {
    Scenario_t *Scenario = Scenario_t::DeserializeScene(Memory->StringArrayGet(NVSScenariosArray, i));
    Automation->AddScenarioCacheItem(Scenario, i);
    delete Scenario;
  }

  ESP_LOGI(tag, "Loaded %i scenarios", ArrayCount);
  delete Memory;
}

string Scenario_t::LoadScenario(uint8_t Index) {
  NVS *Memory = new NVS(NVSAutomationArea);
  return Memory->StringArrayGet(NVSScenariosArray, Index);
  delete Memory;
}

bool Scenario_t::SaveScenario(Scenario_t * Scenario) {
  bool Result = false;

  NVS *Memory = new NVS(NVSAutomationArea);
  uint8_t Index = Memory->StringArrayAdd(NVSScenariosArray, Scenario_t::SerializeScene(Scenario));
  Memory->Commit();

  if (Index != MAX_NVSARRAY_INDEX+1) {
    Automation->AddScenarioCacheItem(Scenario, Index);
    Result = true;
  }

  delete Memory;

  return Result;
}

void Scenario_t::RemoveScenario(uint8_t Index) {
  NVS *Memory = new NVS(NVSAutomationArea);
  Memory->StringArrayRemove(NVSScenariosArray, Index);
  Memory->Commit();
  delete Memory;
}

string Scenario_t::SerializeScene(Scenario_t *Scene) {
  JSON JSONObject;

  JSONObject.SetItems({
                  { "Type"    , Converter::ToHexString(Scene->Type,2) },
                  { "ID"      , Converter::ToHexString(Scene->ID  ,8) },
                  { "Name"    , Scene->Name                           },
                  { "Operand" , Scene->GetDataHexString()             }
                });

  vector<map<string,string>> Commands = vector<map<string,string>>();
  for (int i=0; i < Scene->Commands.size(); i++)
    Commands.push_back({
      { "DeviceID"  , Converter::ToHexString(Scene->Commands[i].DeviceID, 8) },
      { "Command"   , Converter::ToHexString(Scene->Commands[i].CommandID,2) },
      { "Event"     , Converter::ToHexString(Scene->Commands[i].EventCode,2) }
    });

  JSONObject.SetObjectsArray("Commands", Commands);

  return JSONObject.ToString();
}

Scenario_t* Scenario_t::DeserializeScene(string JSONString) {
  JSON JSONObject(JSONString);

  if (!JSONObject.GetItem("type").empty()) {
    Scenario_t *Scene = new Scenario_t(Converter::UintFromHexString<uint8_t>(JSONObject.GetItem("type")));
    Scene->ID = Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("id"));
    Scene->SetData(JSONObject.GetItem("operand"));

    string Name = JSONObject.GetItem("name");
    memcpy(Scene->Name,Name.c_str(),Name.size()+1);

    for (auto &CommandItem : JSONObject.GetObjectsArray("commands")) {
      ScenesCommandItem_t Command;
      Command.DeviceID   = Converter::UintFromHexString<uint32_t>(CommandItem["deviceid"]);
      Command.CommandID  = Converter::UintFromHexString<uint8_t> (CommandItem["command"]);
      Command.EventCode  = Converter::UintFromHexString<uint8_t> (CommandItem["event"]);

      Scene->Commands.push_back(Command);
    }

    return Scene;
  }

  return nullptr;
}


template <size_t ResultSize>
bitset<ResultSize> Scenario_t::Range(bitset<SCENARIOS_OPERAND_BIT_LEN> SrcBitset, size_t Start, size_t Length) {
  bitset<ResultSize> Result;

  if (Start >= 0 && Length > 0 && Start+Length <= SCENARIOS_OPERAND_BIT_LEN)
    for (int i = SCENARIOS_OPERAND_BIT_LEN - Start; i >= SCENARIOS_OPERAND_BIT_LEN - Start - Length; i--)
      Result[i - SCENARIOS_OPERAND_BIT_LEN + Start + Length] = SrcBitset[i];

  return Result;
}

template <size_t SrcSize>
void Scenario_t::AddRangeTo(bitset<SCENARIOS_OPERAND_BIT_LEN> &Destination, bitset<SrcSize> Part, size_t Position) {

  for (int i = SCENARIOS_OPERAND_BIT_LEN - Position - 1; i >= SCENARIOS_OPERAND_BIT_LEN - Position - SrcSize; i--) {
    bool val =  (bool)Part[i - SCENARIOS_OPERAND_BIT_LEN + Position + SrcSize];
    Destination[i] = val;
  }
}
template void Scenario_t::AddRangeTo<4> (bitset<SCENARIOS_OPERAND_BIT_LEN> &Destination, bitset<4>, size_t Position);
template void Scenario_t::AddRangeTo<8> (bitset<SCENARIOS_OPERAND_BIT_LEN> &Destination, bitset<8>, size_t Position);
template void Scenario_t::AddRangeTo<16>(bitset<SCENARIOS_OPERAND_BIT_LEN> &Destination, bitset<16>, size_t Position);
template void Scenario_t::AddRangeTo<32>(bitset<SCENARIOS_OPERAND_BIT_LEN> &Destination, bitset<32>, size_t Position);

bitset<8> Scenario_t::Bitset4To8(bitset<4> Src) {
  bitset<8> Return("0000" + Src.to_string());
  return Return;
}

/************************************/
/*      Scenarios HTTP client       */
/************************************/

void Scenario_t::ReadFinished(char IP[]) {
  ESP_LOGI(tag,"HTTP Command to device with IP %s sent", IP);
}

void Scenario_t::Aborted(char IP[]) {
  ESP_LOGE(tag,"HTTP Command sending to device with IP %s failed", IP);
  Network->SetNetworkDeviceFlagByIP(IP, false);
}

/************************************/
/*        Event Data class          */
/************************************/

bool EventData_t::IsLinked(uint32_t LinkedDeviceID, const vector<ScenesCommandItem_t> &Commands) {
  return (DeviceID == LinkedDeviceID) ? true : false;
}

void EventData_t::SetData(bitset<SCENARIOS_OPERAND_BIT_LEN> Operand) {
  DeviceID          = (uint32_t)Scenario_t::Range<32>(Operand, 0, 32).to_ullong();
  SensorIdentifier  = (uint8_t) Scenario_t::Range<8>(Operand, 32, 8).to_ulong();
  EventCode         = (uint8_t) Scenario_t::Bitset4To8(Scenario_t::Range<4>(Operand, 40, 4)).to_ulong();
}

string EventData_t::ToString() {
  bitset<SCENARIOS_OPERAND_BIT_LEN> Result;

  bitset<32> bDeviceID(DeviceID);
  Scenario_t::AddRangeTo(Result,bDeviceID,0);

  bitset<8> bSensorIdentifier(SensorIdentifier);
  Scenario_t::AddRangeTo(Result,bSensorIdentifier,32);

  bitset<4> bEventCode(EventCode);
  Scenario_t::AddRangeTo(Result,bEventCode,40);

  return Converter::ToHexString(Result.to_ullong(), SCENARIOS_OPERAND_BIT_LEN/4);
}

bool EventData_t::SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t SensorEventCode) {
  return (SensorIdentifier == SensorID && EventCode == SensorEventCode);
};

void EventData_t::ExecuteCommands(uint32_t ScenarioID) {
  Scenario_t::ExecuteScenario(ScenarioID);
};

/************************************/
/*        Timer Data class          */
/************************************/

bool TimerData_t::IsLinked(uint32_t LinkedDeviceID, const vector<ScenesCommandItem_t> &Commands) {
  return (DeviceID == LinkedDeviceID) ? true : false;
}

void TimerData_t::SetData(bitset<SCENARIOS_OPERAND_BIT_LEN> Operand) {
  DeviceID          = (uint32_t)Scenario_t::Range<32>(Operand, 0, 32).to_ulong();
  SensorIdentifier  = (uint8_t) Scenario_t::Range<8> (Operand, 32, 8).to_ulong();
  EventCode         = (uint8_t) Scenario_t::Bitset4To8(Scenario_t::Range<4> (Operand, 40, 4)).to_ulong();
  TimerDelay        = (uint16_t)Scenario_t::Range<16>(Operand, 44, 16).to_ulong();
}

string TimerData_t::ToString() {
  bitset<SCENARIOS_OPERAND_BIT_LEN> Result;

  bitset<32> bDeviceID(DeviceID);
  Scenario_t::AddRangeTo(Result,bDeviceID,0);

  bitset<8> bSensorIdentifier(SensorIdentifier);
  Scenario_t::AddRangeTo(Result,bSensorIdentifier,32);

  bitset<4> bEventCode(EventCode);
  Scenario_t::AddRangeTo(Result,bEventCode,40);

  bitset<16> bTimerDelay(TimerDelay);
  Scenario_t::AddRangeTo(Result,bTimerDelay,44);

  return Converter::ToHexString(Result.to_ullong(), SCENARIOS_OPERAND_BIT_LEN/4);
}

bool TimerData_t::SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t SensorEventCode) {
  return (SensorIdentifier == SensorID && EventCode == SensorEventCode);
};

void TimerData_t::ExecuteCommands(uint32_t ScenarioID) {
  Timer_t *ScenarioTimer = new Timer_t("ScenarioTimer", (TimerDelay * 1000)/portTICK_PERIOD_MS, pdFALSE, ( void * )ScenarioID, TimerCallback);
  ScenarioTimer->Start();
};

void TimerData_t::TimerCallback(Timer_t *pTimer) {
  uint32_t ScenarioID = (uint32_t) pTimer->GetData();
  Scenario_t::ExecuteScenario(ScenarioID);
  ESP_LOGI(tag, "Scenario %s timer executed", Converter::ToHexString(ScenarioID, 8).c_str());
  delete pTimer;
}

/************************************/
/*        Calendar Data class       */
/************************************/

bool CalendarData_t::IsLinked(uint32_t LinkedDeviceID, const vector<ScenesCommandItem_t> &Commands) {
  for (int i=0; i < Commands.size(); i++)
    if (Commands[i].DeviceID == LinkedDeviceID)
      return true;

  return false;
}

void CalendarData_t::SetData(bitset<SCENARIOS_OPERAND_BIT_LEN> Operand) {
  DateTime.Hours    = (uint8_t)Scenario_t::Range<8>(Operand, 0, 8).to_ulong();
  DateTime.Minutes  = (uint8_t)Scenario_t::Range<8>(Operand, 8, 8).to_ulong();
  DateTime.Seconds  = (uint8_t)Scenario_t::Range<8>(Operand,16, 8).to_ulong();

  IsScheduled = Operand[24];

  if (!IsScheduled) {
    DateTime.Day    = (uint8_t)Scenario_t::Range<8>(Operand, 25, 8).to_ulong();
    DateTime.Month  = (uint8_t)Scenario_t::Range<8>(Operand, 33, 8).to_ulong();
    DateTime.Year   = 1970 + (uint8_t)Scenario_t::Range<8>(Operand, 41, 8).to_ulong();
  }
  else {
    ScheduledDays   = Scenario_t::Range<8>(Operand, 24, 8);
    ScheduledDays[7]= false;
  }
}

string CalendarData_t::ToString() {
  bitset<SCENARIOS_OPERAND_BIT_LEN> Result;

  bitset<8> bHours(DateTime.Hours);
  Scenario_t::AddRangeTo(Result, bHours, 0);

  bitset<8> bMinutes(DateTime.Minutes);
  Scenario_t::AddRangeTo(Result, bMinutes, 8);

  bitset<8> bSeconds(DateTime.Seconds);
  Scenario_t::AddRangeTo(Result, bSeconds, 16);

  Result[24] = IsScheduled;

  if (!IsScheduled) {
    bitset<8> bDay(DateTime.Day);
    Scenario_t::AddRangeTo(Result, bDay, 25);

    bitset<8> bMonth(DateTime.Month);
    Scenario_t::AddRangeTo(Result, bMonth, 33);

    bitset<8> bYear(DateTime.Year);
    Scenario_t::AddRangeTo(Result, bYear, 41);
  }
  else {
    Scenario_t::AddRangeTo(Result, ScheduledDays, 24);
    Result[24] = true;
  }

  return Converter::ToHexString(Result.to_ullong(), SCENARIOS_OPERAND_BIT_LEN/4);
}

bool CalendarData_t::IsCommandNeedToExecute(ScenesCommandItem_t &Command) {
  return (Command.DeviceID == Device->ID);
}

void CalendarData_t::ExecuteCommands(uint32_t ScenarioID) {
  Scenario_t::ExecuteScenario(ScenarioID);
};

bool CalendarData_t:: TimeUpdatedIsTriggered() {
  DateTime_t CurrentDateTime = Time::DateTime();

  // DEBUG
  // ESP_LOGI(tag, "Current  Date is %u.%u.%u. DayOfWeek:%u . Time is %u:%u:%u", CurrentDateTime.Day, CurrentDateTime.Month, CurrentDateTime.Year, CurrentDateTime.DayOfWeek, CurrentDateTime.Hours, CurrentDateTime.Minutes, CurrentDateTime.Seconds);
  //ESP_LOGI(tag, "Calendar Date is %u.%u.%u. DayOfWeek:%u. Time is %u:%u:%u", DateTime.Day, DateTime.Month, DateTime.Year, DateTime.DayOfWeek, DateTime.Hours, DateTime.Minutes, DateTime.Seconds);

  DateTime.Hours    = CurrentDateTime.Hours;
  DateTime.Minutes  = CurrentDateTime.Minutes;
  DateTime.Seconds  = CurrentDateTime.Seconds;
  // END DEBUG

  if (CurrentDateTime.Hours == DateTime.Hours &&
      CurrentDateTime.Minutes == DateTime.Minutes &&
      CurrentDateTime.Seconds == DateTime.Seconds) {
        if (IsScheduled) {
          return (bool)ScheduledDays[7-CurrentDateTime.DayOfWeek];
        }
        else if (CurrentDateTime.Day  == DateTime.Day &&
                CurrentDateTime.Month == DateTime.Month &&
                CurrentDateTime.Year  == DateTime.Year)
          return true;
      }

  return false;
}
