/*
*    Scanarios
*    Subpart of API /Automation
*
*/
#include "Globals.h"
#include "Scenarios.h"

static char tag[] = "Scenarios";

/************************************/
/*         Scenario class           */
/************************************/

QueueHandle_t Scenario_t::Queue = FreeRTOS::Queue::Create(Settings.Scenarios.QueueSize, sizeof( uint32_t ));
uint8_t       Scenario_t::ThreadsCounter = 0;

Scenario_t::Scenario_t(uint8_t TypeHex) {
	this->ID			= 0;
	this->Type		= TypeHex;
	this->Commands	= vector<ScenesCommandItem_t>();

	SetType(TypeHex);
}

void Scenario_t::SetType(uint8_t TypeHex) {
	this->Type = TypeHex;

	if (!this->Data)
		delete this->Data;

	switch (Type) {
		case Settings.Scenarios.Types.EventHex     : this->Data = new EventData_t();        break;
		case Settings.Scenarios.Types.TimerHex     : this->Data = new TimerData_t();        break;
		case Settings.Scenarios.Types.CalendarHex  : this->Data = new CalendarData_t();     break;
		default:
			this->Data = new Data_t();
	}
}

Scenario_t::~Scenario_t() {
	if (!Data) delete Data;
}


uint64_t Scenario_t::GetDataUint64() {
	return Data->ToUint64();
}


string Scenario_t::GetDataHexString() {
	return Data->ToString();
}

void Scenario_t::SetData(uint64_t Operand) {
	bitset<Settings.Scenarios.OperandBitLength> DataBitset(Operand);
	Data->SetData(DataBitset);
}

void Scenario_t::SetData(string HexString) {
	if (HexString.length() > 16) HexString.substr(0, 16);
	if (HexString.length() < 16) while (HexString.length() < 16) HexString += "0";

	bitset<Settings.Scenarios.OperandBitLength> DataBitset(Converter::UintFromHexString<uint64_t>(HexString));
	Data->SetData(DataBitset);
}

bool Scenario_t::IsEmpty() {
	return (ID == ~Settings.Memory.Empty32Bit || ID == Settings.Memory.Empty32Bit || Type == 0xFF) ? true : false;
}

void Scenario_t::ExecuteScenario(uint32_t ScenarioID) {
	FreeRTOS::Queue::SendToBack(Scenario_t::Queue, &ScenarioID, (TickType_t) Settings.Scenarios.BlockTicks );

	if (ThreadsCounter <= 0 && FreeRTOS::Queue::Count(Scenario_t::Queue)) {
		ThreadsCounter = 1;
		FreeRTOS::StartTask(ExecuteCommandsTask, "ExecuteCommandsTask", ( void * )(uint32_t)ThreadsCounter, Settings.Scenarios.TaskStackSize);
	}
}

void Scenario_t::ExecuteCommandsTask(void *TaskData) {
  ESP_LOGD(tag, "Scenario executed task %u created", (uint32_t) TaskData);

  uint32_t ScenarioID = 0;

  if (Queue != 0)
    while (FreeRTOS::Queue::Receive(Scenario_t::Queue, &ScenarioID, (TickType_t) Settings.Scenarios.TaskStackSize)) {
      Scenario_t Scenario;

      LoadScenario(Scenario,ScenarioID);

      for (ScenesCommandItem_t Command : Scenario.Commands)
        if (Scenario.Data->IsCommandNeedToExecute(Command)) {
          if (Command.DeviceID == Settings.eFuse.DeviceID) {
            // выполнить локальную команду
            Command_t *CommandToExecute = Command_t::GetCommandByID(Command.CommandID);
            if (CommandToExecute!=nullptr)
              CommandToExecute->Execute(Command.EventCode, Converter::ToString((uint32_t)Command.Operand));
          }
          else {
            // отправить команду по HTTP
            NetworkDevice_t NetworkDevice = Network.GetNetworkDeviceByID(Command.DeviceID);

            if (!NetworkDevice.IP.empty() && NetworkDevice.IsActive) {
              string URL = "/commands?command=" + Converter::ToHexString(Command.CommandID,2) +
                                     "&action=" + Converter::ToHexString(Command.EventCode,2) +
                                    "&operand=" + Converter::ToHexString(Command.Operand, 8);

              HTTPClient::Query("", 80, URL, QueryType::GET, NetworkDevice.IP, false, NULL, NULL, &ReadFinished, &Aborted);
            }
          }
        }
    }

  ESP_LOGD(tag, "Task %u removed", (uint32_t)TaskData);
  Scenario_t::ThreadsCounter--;
  FreeRTOS::DeleteTask();
}

void Scenario_t::LoadScenarios() {
	uint32_t Address 			= Settings.Scenarios.Memory.Start;
	uint32_t ScenarioFindedID 	= 0x0;

	while (Address < Settings.Scenarios.Memory.Start + Settings.Scenarios.Memory.Size) {
		ScenarioFindedID = SPIFlash::ReadUint32(Address);

		if (ScenarioFindedID != Settings.Memory.Empty32Bit) {
			Scenario_t Scenario;

			LoadScenarioByAddress(Scenario, Address);

			if (!Scenario.IsEmpty())
				Automation.AddScenarioCacheItem(Scenario);
		}
		Address += Settings.Scenarios.Memory.ItemSize;
	}

	ESP_LOGI(tag, "Loaded %d scenarios", Automation.ScenarioCacheItemCount());
}

void Scenario_t::LoadScenario(Scenario_t &Scenario, uint32_t ScenarioID) {
	uint32_t Address 			= Settings.Scenarios.Memory.Start;

	while (Address < Settings.Scenarios.Memory.Start + Settings.Scenarios.Memory.Size) {
		uint32_t FindedID = SPIFlash::ReadUint32(Address);

		if (FindedID == ScenarioID)
			LoadScenarioByAddress(Scenario, Address);

		Address += Settings.Scenarios.Memory.ItemSize;
	}
}

void Scenario_t::LoadScenarioByAddress(Scenario_t &Scenario, uint32_t Address) {
	Scenario.ID = SPIFlash::ReadUint32(Address + Settings.Scenarios.Memory.ItemOffset.ID);

	Scenario.SetType(SPIFlash::ReadUint8(Address + Settings.Scenarios.Memory.ItemOffset.Type));

	if (Scenario.ID == Settings.Memory.Empty32Bit)
		return;

	Scenario.SetData(SPIFlash::ReadUint64(Address + Settings.Scenarios.Memory.ItemOffset.Operand));

	for (int i = 0; i< Settings.Scenarios.NameLength; i++)
		Scenario.Name[i] = SPIFlash::ReadUint16(Address + Settings.Scenarios.Memory.ItemOffset.Name + i*0x2);

	uint32_t CommandAddress 	= Address + Settings.Scenarios.Memory.ItemOffset.Commands;

	while (CommandAddress < Address + Settings.Scenarios.Memory.ItemSize) {
		uint32_t FindedCommandDeviceID = SPIFlash::ReadUint32(CommandAddress + Settings.Scenarios.Memory.CommandOffset.DeviceID);

		if (FindedCommandDeviceID == Settings.Memory.Empty32Bit)
			break;

		ScenesCommandItem_t Command;
		Command.DeviceID 	= FindedCommandDeviceID;
		Command.CommandID 	= SPIFlash::ReadUint8 (CommandAddress + Settings.Scenarios.Memory.CommandOffset.CommandID);
		Command.EventCode	= SPIFlash::ReadUint8 (CommandAddress + Settings.Scenarios.Memory.CommandOffset.EventID);
		Command.Operand		= SPIFlash::ReadUint32(CommandAddress + Settings.Scenarios.Memory.CommandOffset.Operand);

		Scenario.Commands.push_back(Command);

		CommandAddress += Settings.Scenarios.Memory.CommandOffset.Size;
	}
}

bool Scenario_t::SaveScenario(Scenario_t Scenario) {
	uint32_t Address = Settings.Scenarios.Memory.Start;

	while (Address < Settings.Scenarios.Memory.Start + Settings.Scenarios.Memory.Size) {
		uint32_t CurrentID = SPIFlash::ReadUint32(Address);

		if (CurrentID == Settings.Memory.Empty32Bit)
			break;

		Address += Settings.Scenarios.Memory.ItemSize;
	}

	if (Address > Settings.Scenarios.Memory.Start + Settings.Scenarios.Memory.Size - 1)
		return false;

	if (Scenario.IsEmpty())
		return false;

	SPIFlash::WriteUint32(Scenario.ID	, Address + Settings.Scenarios.Memory.ItemOffset.ID);
	SPIFlash::WriteUint8 (Scenario.Type	, Address + Settings.Scenarios.Memory.ItemOffset.Type);

	for (int i=0; i< Settings.Scenarios.NameLength; i++)
		SPIFlash::WriteUint16(Scenario.Name[i], Address + Settings.Scenarios.Memory.ItemOffset.Name + i*0x2);

	SPIFlash::WriteUint64(Scenario.Data->ToUint64(), Address + Settings.Scenarios.Memory.ItemOffset.Operand);

	//NAME SAVE
	uint32_t CommandAddress 	= Address + Settings.Scenarios.Memory.ItemOffset.Commands;

	for (int i=0 ; i < Scenario.Commands.size(); i++) {
		CommandAddress += i*Settings.Scenarios.Memory.CommandOffset.Size;

		SPIFlash::WriteUint32(Scenario.Commands[i].DeviceID	, CommandAddress + Settings.Scenarios.Memory.CommandOffset.DeviceID);
		SPIFlash::WriteUint8 (Scenario.Commands[i].CommandID	, CommandAddress + Settings.Scenarios.Memory.CommandOffset.CommandID);
		SPIFlash::WriteUint8 (Scenario.Commands[i].EventCode	, CommandAddress + Settings.Scenarios.Memory.CommandOffset.EventID);
		SPIFlash::WriteUint32(Scenario.Commands[i].Operand	, CommandAddress + Settings.Scenarios.Memory.CommandOffset.Operand);
	}

	Automation.AddScenarioCacheItem(Scenario);

	return true;
}

void Scenario_t::RemoveScenario(uint32_t ScenarioID) {
	uint32_t Address 			= Settings.Scenarios.Memory.Start;

	while (Address < Settings.Scenarios.Memory.Start + Settings.Scenarios.Memory.Size) {
		uint32_t FindedID = SPIFlash::ReadUint32(Address);

		if (FindedID == ScenarioID)
			SPIFlash::EraseRange(Address, Settings.Scenarios.Memory.ItemSize);

		Address += Settings.Scenarios.Memory.ItemSize;
	}
}

string Scenario_t::SerializeScene(uint32_t ScenarioID) {
	Scenario_t Scenario;
	LoadScenario(Scenario, ScenarioID);

	return (!Scenario.IsEmpty()) ? SerializeScene(Scenario) : "";
}

string Scenario_t::SerializeScene(Scenario_t Scenario) {
	JSON JSONObject;

	JSONObject.SetItems(vector<pair<string,string>>({
		make_pair("Type"	, Converter::ToHexString(Scenario.Type,2)),
		make_pair("ID"		, Converter::ToHexString(Scenario.ID  ,8)),
		make_pair("Name"	, Converter::ToUTF16String(Scenario.Name, Settings.Scenarios.NameLength)),
		make_pair("Operand"	, Scenario.GetDataHexString()),
	}));

	vector<string> Commands = vector<string>();
	for (int i=0; i < Scenario.Commands.size(); i++)
		Commands.push_back(	Converter::ToHexString(Scenario.Commands[i].DeviceID ,8) +
							Converter::ToHexString(Scenario.Commands[i].CommandID,2) +
							Converter::ToHexString(Scenario.Commands[i].EventCode,2) +
							Converter::ToHexString(Scenario.Commands[i].Operand  ,8));

	JSONObject.SetStringArray("Commands", Commands);

	return JSONObject.ToString();
}

Scenario_t Scenario_t::DeserializeScene(string JSONString) {
	JSON JSONObject(JSONString);

	if (!JSONObject.GetItem("type").empty()) {
		Scenario_t Scene = Scenario_t(Converter::UintFromHexString<uint8_t>(JSONObject.GetItem("type")));
		Scene.ID = Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("id"));
		Scene.SetData(JSONObject.GetItem("operand"));

		vector<uint16_t> UTF16Name = Converter::ToUTF16Vector(JSONObject.GetItem("name"));

		for (int i=0; i < UTF16Name.size() && i < Settings.Scenarios.NameLength; i++)
			Scene.Name[i] = UTF16Name[i];

		for (auto &CommandString : JSONObject.GetStringArray("commands")) {
			ScenesCommandItem_t Command;
			Command.DeviceID   = Converter::UintFromHexString<uint32_t>(CommandString.substr(0 ,8));
			Command.CommandID  = Converter::UintFromHexString<uint8_t> (CommandString.substr(8 ,2));
			Command.EventCode  = Converter::UintFromHexString<uint8_t> (CommandString.substr(10,2));
			Command.Operand    = Converter::UintFromHexString<uint8_t> (CommandString.substr(12,8));

			Scene.Commands.push_back(Command);
		}

		return Scene;
	}

	return Scenario_t(0);
}


template <size_t ResultSize>
bitset<ResultSize> Scenario_t::Range(bitset<Settings_t::Scenarios_t::OperandBitLength> SrcBitset, size_t Start, size_t Length) {
  bitset<ResultSize> Result;

  if (Start >= 0 && Length > 0 && Start+Length <= Settings.Scenarios.OperandBitLength)
    for (int i = Settings.Scenarios.OperandBitLength - Start; i >= Settings.Scenarios.OperandBitLength - Start - Length; i--)
      Result[i - Settings.Scenarios.OperandBitLength + Start + Length] = SrcBitset[i];

  return Result;
}

template <size_t SrcSize>
void Scenario_t::AddRangeTo(bitset<Settings_t::Scenarios_t::OperandBitLength> &Destination, bitset<SrcSize> Part, size_t Position) {

  for (int i = Settings.Scenarios.OperandBitLength - Position - 1; i >= Settings.Scenarios.OperandBitLength - Position - SrcSize; i--) {
    bool val =  (bool)Part[i - Settings.Scenarios.OperandBitLength + Position + SrcSize];
    Destination[i] = val;
  }
}
template void Scenario_t::AddRangeTo<4> (bitset<Settings.Scenarios.OperandBitLength> &Destination, bitset<4>, size_t Position);
template void Scenario_t::AddRangeTo<8> (bitset<Settings.Scenarios.OperandBitLength> &Destination, bitset<8>, size_t Position);
template void Scenario_t::AddRangeTo<12>(bitset<Settings.Scenarios.OperandBitLength> &Destination, bitset<12>, size_t Position);
template void Scenario_t::AddRangeTo<16>(bitset<Settings.Scenarios.OperandBitLength> &Destination, bitset<16>, size_t Position);
template void Scenario_t::AddRangeTo<32>(bitset<Settings.Scenarios.OperandBitLength> &Destination, bitset<32>, size_t Position);

/************************************/
/*      Scenarios HTTP client       */
/************************************/

void Scenario_t::ReadFinished(char IP[]) {
	ESP_LOGI(tag,"HTTP Command to device with IP %s sent", IP);
}

void Scenario_t::Aborted(char IP[]) {
	ESP_LOGE(tag,"HTTP Command sending to device with IP %s failed", IP);
	Network.SetNetworkDeviceFlagByIP(IP, false);
}

/************************************/
/*        Event Data class          */
/************************************/

bool EventData_t::IsLinked(uint32_t LinkedDeviceID, const vector<ScenesCommandItem_t> &Commands) {
	return (DeviceID == LinkedDeviceID) ? true : false;
}

void EventData_t::SetData(bitset<Settings.Scenarios.OperandBitLength> Operand) {
	DeviceID          = (uint32_t)Scenario_t::Range<32>(Operand, 0, 32).to_ullong();
	SensorIdentifier  = (uint8_t) Scenario_t::Range<8> (Operand, 32, 8).to_ulong();
	EventCode         = (uint8_t) Scenario_t::Range<8> (Operand, 40, 8).to_ulong();
	EventOperand      = (uint8_t) Scenario_t::Range<8> (Operand, 48, 8).to_ulong();
}

uint64_t EventData_t::ToUint64() {
	bitset<Settings.Scenarios.OperandBitLength> Result;

	bitset<32> bDeviceID(DeviceID);
	Scenario_t::AddRangeTo(Result,bDeviceID,0);

	bitset<8> bSensorIdentifier(SensorIdentifier);
	Scenario_t::AddRangeTo(Result,bSensorIdentifier,32);

	bitset<8> bEventCode(EventCode);
	Scenario_t::AddRangeTo(Result,bEventCode,40);

	bitset<8> bOperand(EventOperand);
	Scenario_t::AddRangeTo(Result,bOperand,48);

	return Result.to_ullong();
}

string EventData_t::ToString() {
	return Converter::ToHexString(this->ToUint64(), Settings.Scenarios.OperandBitLength/4);
}


bool EventData_t::SensorUpdatedIsTriggered(uint8_t SensorID) {
	if (SensorIdentifier == SensorID)
		return Sensor_t::GetSensorByID(SensorID)->CheckOperand(EventCode, EventOperand);

	return false;
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

void TimerData_t::SetData(bitset<Settings.Scenarios.OperandBitLength> Operand) {
	DeviceID				= (uint32_t)Scenario_t::Range<32>(Operand, 0, 32).to_ulong();
	SensorIdentifier		= (uint8_t) Scenario_t::Range<8> (Operand, 32, 8).to_ulong();
	EventCode			= (uint8_t) Scenario_t::Range<8> (Operand, 40, 8).to_ulong();
	TimerDelay			= (uint16_t)Scenario_t::Range<8> (Operand, 48, 8).to_ulong();
	EventOperand			= (uint8_t) Scenario_t::Range<8> (Operand << 56, 0, 8).to_ulong();

	if (TimerDelay >= 10)
		TimerDelay		= (TimerDelay - 8) * 5;
}

uint64_t TimerData_t::ToUint64() {
	bitset<Settings.Scenarios.OperandBitLength> Result;

	bitset<8> bOperand(EventOperand);
	Scenario_t::AddRangeTo(Result,bOperand,0);
	Result >>= 56;

	bitset<32> bDeviceID(DeviceID);
	Scenario_t::AddRangeTo(Result,bDeviceID,0);

	bitset<8> bSensorIdentifier(SensorIdentifier);
	Scenario_t::AddRangeTo(Result,bSensorIdentifier,32);

	bitset<8> bEventCode(EventCode);
	Scenario_t::AddRangeTo(Result,bEventCode,40);

	bitset<8> bTimerDelay((uint8_t)(TimerDelay < 10) ? TimerDelay : (TimerDelay / 5) + 8);
	Scenario_t::AddRangeTo(Result,bTimerDelay,48);

	return Result.to_ullong();
}

string TimerData_t::ToString() {
	return Converter::ToHexString(this->ToUint64(), Settings.Scenarios.OperandBitLength/4);
}

bool TimerData_t::SensorUpdatedIsTriggered(uint8_t SensorID) {
  if (SensorIdentifier == SensorID) {
    return Sensor_t::GetSensorByID(SensorID)->CheckOperand(EventCode, EventOperand);
  }

  return false;
};

void TimerData_t::ExecuteCommands(uint32_t ScenarioID) {
	FreeRTOS::Timer *ScenarioTimer = new FreeRTOS::Timer("ScenarioTimer", (TimerDelay * 1000)/portTICK_PERIOD_MS, pdFALSE, ( void * )ScenarioID, TimerCallback);
	ScenarioTimer->Start();
};

void TimerData_t::TimerCallback(FreeRTOS::Timer *pTimer) {
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

void CalendarData_t::SetData(bitset<Settings.Scenarios.OperandBitLength> Operand) {
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

uint64_t CalendarData_t::ToUint64() {
	bitset<Settings.Scenarios.OperandBitLength> Result;

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

	return Result.to_ullong();
}

string CalendarData_t::ToString() {
	return Converter::ToHexString(this->ToUint64(), Settings.Scenarios.OperandBitLength/4);
}

bool CalendarData_t::IsCommandNeedToExecute(ScenesCommandItem_t &Command) {
	return (Command.DeviceID == Settings.eFuse.DeviceID);
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
