/*
*    Scanarios
*    Subpart of API /Automation
*
*/

#include "Scenarios.h"

#include <algorithm>
#include <esp_log.h>

#include "JSON.h"

#include "Converter.h"
#include "Globals.h"


static char tag[] = "Scenarios";

/************************************/
/*         Scenario class           */
/************************************/

map<string, vector<ScenesCommandItem_t>>
              Scenario_t::CommandsCacheMap = map<string, vector<ScenesCommandItem_t>>();
map<string,TaskHandle_t>
              Scenario_t::CommandsCacheTaskHandleMap = map<string,TaskHandle_t>();

Scenario_t::Scenario_t(uint8_t TypeHex) {
  this->Type                  = TypeHex;
  this->Commands              = vector<ScenesCommandItem_t>();

  switch (Type) {
    case SCENARIO_TYPE_EVENT_HEX     : this->Data = new EventData_t();        break;
    case SCENARIO_TYPE_TIMER_HEX     : this->Data = new TimerData_t();        break;
    case SCENARIO_TYPE_CALENDAR_HEX  : this->Data = new CalendarData_t();     break;
  }
}

string Scenario_t::GetDataHexString() {
  return Data->ToString();
}

void Scenario_t::SetData(uint64_t Operand) {
  bitset<SCENARIO_OPERAND_BIT_LEN> DataBitset(Operand);
  Data->SetData(DataBitset);
}

void Scenario_t::SetData(string HexString) {
  bitset<SCENARIO_OPERAND_BIT_LEN> DataBitset(Converter::UintFromHexString<uint64_t>(HexString));
  Data->SetData(DataBitset);
}

bool Scenario_t::Empty() {
  return (ID == 0 && Type == 0x00 && string(Name).empty()) ? true : false;
}

void Scenario_t::ExecuteCommands(uint32_t ScenarioID) {
  TaskHandle_t CurrentHandle = FreeRTOS::StartTask(ExecuteCommandsTask, "ExecuteCommandsTask", ( void * ) ScenarioID, 4096);
  CommandsCacheTaskHandleSet(ScenarioID, CurrentHandle);
}

void Scenario_t::ExecuteCommandsTask(void *Data) {
  uint32_t ScenarioID = (uint32_t)Data;

  if (ScenarioID > 0) {
    vector<ScenesCommandItem_t> CommandsToExecute = CommandsCacheGet(ScenarioID);

    for (auto& Command : CommandsToExecute) {
        if (Command.DeviceID == Device->ID) {
          // выполнить локальную команду
          Command_t *CommandToExecute = Command_t::GetCommandByID(Command.CommandID);
          if (CommandToExecute!=nullptr)
            CommandToExecute->Execute(Command.EventCode, Command.Operand);
          //delete CommandToExecute
        }
        else {
          // отправить команду по HTTP
          NetworkDevice_t NetworkDevice = Network->GetNetworkDeviceByID(Command.DeviceID);

          if (!NetworkDevice.IP.empty() && NetworkDevice.IsActive) {

            ScenariosHTTPClient_t* HTTPClient  = new ScenariosHTTPClient_t();

            HTTPClient->IP            = NetworkDevice.IP;
            HTTPClient->ContentURI    = "/commands?command=" +
                                        Converter::ToHexString(Command.CommandID,2) +
                                        "&action=" +
                                        Converter::ToHexString(Command.EventCode,2) +
                                        "&operand=" +
                                        Converter::ToHexString(Command.Operand, 8);

            HTTPClient->Request();

            delete HTTPClient;
          }
        }
    }
  }

  FreeRTOS::DeleteTask(Scenario_t::CommandsCacheTaskHandleGet(ScenarioID));
  CommandsCacheErase(ScenarioID);
  CommandsCacheTaskHandleErase(ScenarioID);
}

void Scenario_t::CommandsCacheSet(uint32_t ScenarioID, vector<ScenesCommandItem_t> CommandsToExecute) {
  CommandsCacheMap[Converter::ToHexString(ScenarioID, 8)] = CommandsToExecute;
}

vector<ScenesCommandItem_t> Scenario_t::CommandsCacheGet(uint32_t ScenarioID) {
  string Key = Converter::ToHexString(ScenarioID, 8);

  if (CommandsCacheMap.count(Key) > 0)
    return CommandsCacheMap[Key];
  else
    return vector<ScenesCommandItem_t>();
}

void Scenario_t::CommandsCacheErase(uint32_t ScenarioID) {
  CommandsCacheMap.erase(Converter::ToHexString(ScenarioID, 8));
}


void Scenario_t::CommandsCacheTaskHandleSet(uint32_t ScenarioID, TaskHandle_t TaskHandle) {
  CommandsCacheTaskHandleMap[Converter::ToHexString(ScenarioID, 8)] = TaskHandle;
}

TaskHandle_t Scenario_t::CommandsCacheTaskHandleGet(uint32_t ScenarioID) {
  string Key = Converter::ToHexString(ScenarioID, 8);
  if (Scenario_t::CommandsCacheTaskHandleMap.count(Key) > 0)
    return Scenario_t::CommandsCacheTaskHandleMap[Key];
  else
    return nullptr;
}

void Scenario_t::CommandsCacheTaskHandleErase(uint32_t ScenarioID) {
  Scenario_t::CommandsCacheTaskHandleMap.erase(Converter::ToHexString(ScenarioID, 8));
}

string Scenario_t::SerializeScene(Scenario_t *Scene) {
  JSON_t *JSON = new JSON_t();

  JSON->SetParam({
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

  JSON->SetMapsArray("Commands", Commands);

  string Result = JSON->ToString();
  delete JSON;

  return Result;
}

Scenario_t* Scenario_t::DeserializeScene(string JSONString) {
  JSON_t *JSON = new JSON_t(JSONString);
  if (!JSON->GetParam("type").empty()) {
    Scenario_t *Scene = new Scenario_t(Converter::UintFromHexString<uint8_t>(JSON->GetParam("type")));
    Scene->ID   = Converter::UintFromHexString<uint32_t>(JSON->GetParam("id"));
    Scene->SetData(JSON->GetParam("operand"));

    string Name = JSON->GetParam("name");
    memcpy(Scene->Name,Name.c_str(),Name.size()+1);

    for (int i=0; i < JSON->GetMapsArray("commands").size(); i++) {
      ScenesCommandItem_t Command;
      Command.DeviceID   = Converter::UintFromHexString<uint32_t>(JSON->GetMapsArray("commands")[i]["deviceid"]);
      Command.CommandID  = Converter::UintFromHexString<uint8_t> (JSON->GetMapsArray("commands")[i]["command"]);
      Command.EventCode  = Converter::UintFromHexString<uint8_t> (JSON->GetMapsArray("commands")[i]["event"]);

      Scene->Commands.push_back(Command);
    }

    delete JSON;
    return Scene;
  }

  delete JSON;
  return nullptr;
}

template <size_t ResultSize>
bitset<ResultSize> Scenario_t::Range(bitset<SCENARIO_OPERAND_BIT_LEN> SrcBitset, size_t Start, size_t Length) {
  bitset<ResultSize> Result;

  if (Start >= 0 && Length > 0 && Start+Length <= SCENARIO_OPERAND_BIT_LEN)
    for (int i = SCENARIO_OPERAND_BIT_LEN - Start; i >= SCENARIO_OPERAND_BIT_LEN - Start - Length; i--)
      Result[i - SCENARIO_OPERAND_BIT_LEN + Start + Length] = SrcBitset[i];

  return Result;
}

template <size_t SrcSize>
void Scenario_t::AddRangeTo(bitset<SCENARIO_OPERAND_BIT_LEN> &Destination, bitset<SrcSize> Part, size_t Position) {

  for (int i = SCENARIO_OPERAND_BIT_LEN - Position - 1; i >= SCENARIO_OPERAND_BIT_LEN - Position - SrcSize; i--) {
    bool val =  (bool)Part[i - SCENARIO_OPERAND_BIT_LEN + Position + SrcSize];
    Destination[i] = val;
  }
}
template void Scenario_t::AddRangeTo<4> (bitset<SCENARIO_OPERAND_BIT_LEN> &Destination, bitset<4>, size_t Position);
template void Scenario_t::AddRangeTo<8> (bitset<SCENARIO_OPERAND_BIT_LEN> &Destination, bitset<8>, size_t Position);
template void Scenario_t::AddRangeTo<16>(bitset<SCENARIO_OPERAND_BIT_LEN> &Destination, bitset<16>, size_t Position);
template void Scenario_t::AddRangeTo<32>(bitset<SCENARIO_OPERAND_BIT_LEN> &Destination, bitset<32>, size_t Position);

bitset<8> Bitset4To8(bitset<4> Src) {
  bitset<8> Return("0000" + Src.to_string());
  return Return;
}

/************************************/
/*      Scenarios HTTP client       */
/************************************/

bool ScenariosHTTPClient_t::ReadFinished() {
  ESP_LOGI(tag,"HTTP Command sent");
  return true;
}

void ScenariosHTTPClient_t::Aborted() {
  ESP_LOGE(tag,"HTTP Command sending failed");
}

/************************************/
/*        Event Data class          */
/************************************/

bool EventData_t::IsLinked(uint32_t LinkedDeviceID, const vector<ScenesCommandItem_t> &Commands) {
  return (DeviceID == LinkedDeviceID) ? true : false;
}

void EventData_t::SetData(bitset<SCENARIO_OPERAND_BIT_LEN> Operand) {
  DeviceID          = (uint32_t)Scenario_t::Range<32>(Operand, 0, 32).to_ullong();
  SensorIdentifier  = (uint8_t) Scenario_t::Range<8>(Operand, 32, 8).to_ulong();
  EventCode         = (uint8_t) Bitset4To8(Scenario_t::Range<4>(Operand, 40, 4)).to_ulong();
}

string EventData_t::ToString() {
  bitset<SCENARIO_OPERAND_BIT_LEN> Result;

  bitset<32> bDeviceID(DeviceID);
  Scenario_t::AddRangeTo(Result,bDeviceID,0);

  bitset<8> bSensorIdentifier(SensorIdentifier);
  Scenario_t::AddRangeTo(Result,bSensorIdentifier,32);

  bitset<4> bEventCode(EventCode);
  Scenario_t::AddRangeTo(Result,bEventCode,40);

  return Converter::ToHexString(Result.to_ullong(), SCENARIO_OPERAND_BIT_LEN/4);
}

bool EventData_t::SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t SensorEventCode) {
  return (SensorIdentifier == SensorID && EventCode == SensorEventCode);
};

void EventData_t::ExecuteCommands(const vector<ScenesCommandItem_t> CommandsToExecute, uint32_t ScenarioID) {
  if (ScenarioID > 0) {
    Scenario_t::CommandsCacheSet(ScenarioID, CommandsToExecute);
    Scenario_t::ExecuteCommands(ScenarioID);
  }
};

/************************************/
/*        Timer Data class          */
/************************************/

bool TimerData_t::IsLinked(uint32_t LinkedDeviceID, const vector<ScenesCommandItem_t> &Commands) {
  return (DeviceID == LinkedDeviceID) ? true : false;
}

void TimerData_t::SetData(bitset<SCENARIO_OPERAND_BIT_LEN> Operand) {
  DeviceID          = (uint32_t)Scenario_t::Range<32>(Operand, 0, 32).to_ulong();
  SensorIdentifier  = (uint8_t) Scenario_t::Range<8> (Operand, 32, 8).to_ulong();
  EventCode         = (uint8_t) Bitset4To8(Scenario_t::Range<4> (Operand, 40, 4)).to_ulong();
  TimerDelay        = (uint16_t)Scenario_t::Range<16>(Operand, 44, 16).to_ulong();
}

string TimerData_t::ToString() {
  bitset<SCENARIO_OPERAND_BIT_LEN> Result;

  bitset<32> bDeviceID(DeviceID);
  Scenario_t::AddRangeTo(Result,bDeviceID,0);

  bitset<8> bSensorIdentifier(SensorIdentifier);
  Scenario_t::AddRangeTo(Result,bSensorIdentifier,32);

  bitset<4> bEventCode(EventCode);
  Scenario_t::AddRangeTo(Result,bEventCode,40);

  bitset<16> bTimerDelay(TimerDelay);
  Scenario_t::AddRangeTo(Result,bTimerDelay,44);

  return Converter::ToHexString(Result.to_ullong(), SCENARIO_OPERAND_BIT_LEN/4);
}

bool TimerData_t::SensorUpdatedIsTriggered(uint8_t SensorID, uint8_t SensorEventCode) {
  return (SensorIdentifier == SensorID && EventCode == SensorEventCode);
};

void TimerData_t::ExecuteCommands(const vector<ScenesCommandItem_t> CommandsToExecute, uint32_t ScenarioID) {
  if (ScenarioID != 0) {
    Scenario_t::CommandsCacheSet(ScenarioID, CommandsToExecute);
    Timer_t *ScenarioTimer = new Timer_t("ScenarioTimer", (TimerDelay*1000)/portTICK_PERIOD_MS, pdFALSE, ( void * )ScenarioID, TimerCallback);
    ScenarioTimer->Start();
  }
};

void TimerData_t::TimerCallback(Timer_t *pTimer) {
  uint32_t ScenarioID =  (uint32_t) pTimer->GetData();
  Scenario_t::ExecuteCommands(ScenarioID);
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

void CalendarData_t::SetData(bitset<SCENARIO_OPERAND_BIT_LEN> Operand) {
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
  bitset<SCENARIO_OPERAND_BIT_LEN> Result;

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

  return Converter::ToHexString(Result.to_ullong(), SCENARIO_OPERAND_BIT_LEN/4);
}

void CalendarData_t::ExecuteCommands(const vector<ScenesCommandItem_t> CommandsToExecute, uint32_t ScenarioID) {
  if (ScenarioID != 0) {

    vector<ScenesCommandItem_t> NeededCommands = CommandsToExecute;
    for (vector<ScenesCommandItem_t>::iterator it = NeededCommands.begin() ; it != NeededCommands.end(); ++it)
      if ((*it).DeviceID != Device->ID)
        NeededCommands.erase (it);

    Scenario_t::CommandsCacheSet(ScenarioID, NeededCommands);
    Scenario_t::ExecuteCommands(ScenarioID);
  }
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
