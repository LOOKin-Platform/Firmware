/*
*    Log.cpp
*    Class to handle API /Log
*
*/

#include "Log.h"

static char tag[] = "Log";
vector<Log::Item> Log::Items = vector<Log::Item>();

Log::Log() {}

/**
 * @brief Add log record
 *
 * @param [in] Code log event code.
 * @param [in] Data log event Data if exist.
 */
void Log::Add(uint16_t Code, uint32_t Data) {
  Log::Item Record;

  Record.Code = Code;
  Record.Data = Data;
  Record.Time = Time::Unixtime();

  if (GetItemType(Code) == SYSTEM) { // важное системные события
    NVS Memory(NVSLogArea);
    uint8_t Index = Memory.StringArrayAdd(NVSLogArray, Serialize(Record));

    if (Index > Settings.Log.SystemLogSize)
      Memory.StringArrayRemove(NVSLogArray, 0);

    Memory.Commit();
  }
  else { // событие, не требующее хранение в NVS
    Items.push_back(Record);

    if (Items.size() >= Settings.Log.EventsLogSize)
      Items.erase(Items.begin());

    if (GetItemType(Code) == ERROR)
      ESP_LOGE(tag, "Error code %04X(%08X)", Code, Data);

    if (GetItemType(Code) == INFO)
      ESP_LOGI(tag, "Info code %04X(%08X)", Code, Data);
  }
}

/**
 * @brief Get system log data
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
vector<Log::Item> Log::GetSystemLog() {
  vector<Item> Result = vector<Item>();

  NVS *Memory = new NVS(NVSLogArea);

  for (int i = GetSystemLogCount(Memory) - 1; i > 0; i--)
    Result.push_back(GetSystemLogItem(i));

  delete Memory;

  return Result;
}

/**
 * @brief Get system log records count
 *
 * @return log items count
 */
uint8_t Log::GetSystemLogCount(NVS *MemoryLink) {
	uint8_t Count = 0;

	if (MemoryLink == nullptr) {
		NVS Memory(NVSLogArea);
		Count = Memory.ArrayCount(NVSLogArray);
	}
	else
	  Count = MemoryLink->ArrayCount(NVSLogArray);

	return Count;
}

/**
 * @brief Get system log record by index
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
Log::Item Log::GetSystemLogItem(uint8_t Index,NVS *MemoryLink) {
	Item Result;

	if (MemoryLink == nullptr) {
		Result = Deserialize(GetSystemLogJSONItem(Index));
	}
	else
		Result = Deserialize(GetSystemLogJSONItem(Index, MemoryLink));

	return Result;
}

/**
 * @brief Get system log record in JSON by index
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
string Log::GetSystemLogJSONItem(uint8_t Index, NVS *MemoryLink) {
	string Result;

	if (MemoryLink == nullptr) {
		NVS Memory(NVSLogArea);
		Result = Memory.StringArrayGet(NVSLogArray, Index);
	}
	else
		Result = MemoryLink->StringArrayGet(NVSLogArray, Index);

	return Result;
}

/**
 * @brief Get JSON string with all exists system log records
 * @return JSON string contains log items.
 */
string Log::GetSystemLogJSON() {
	vector<string> Result = vector<string>();
	NVS *Memory = new NVS(NVSLogArea);

	for (int i = GetSystemLogCount(Memory) - 1; i > 0; i--)
		Result.push_back(GetSystemLogJSONItem(i, Memory));

	delete Memory;

	return "[" + Converter::VectorToString(Result, ",") + "]";
}

/**
 * @brief Get events log data
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
vector<Log::Item> Log::GetEventsLog() {
    return Items;
}

/**
 * @brief Get events log records count
 *
 * @return log items count
 */
uint8_t Log::GetEventsLogCount() {
	return Items.size();
}

/**
 * @brief Get events log record by index
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
Log::Item Log::GetEventsLogItem(uint8_t Index) {
	if (Index < Items.size())
		return Items.at(Index);
	else
		return Log::Item{};
}


/**
 * @brief Get system log record in JSON by index
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
string Log::GetEventsLogJSONItem(uint8_t Index) {
	JSON JSONObject;

	JSONObject.SetItem("Code", Converter::ToHexString(Items.at(Index).Code, 4));
	JSONObject.SetItem("Data", Converter::ToHexString(Items.at(Index).Data, 8));
	JSONObject.SetItem("Time", Converter::ToHexString(Items.at(Index).Time, 8));

	return JSONObject.ToString();
}


/**
 * @brief Get JSON string with all exists system log records
 * @return JSON string contains log items.
 */
string Log::GetEventsLogJSON() {
	vector<string> Result = vector<string>();

	for (int i = Items.size() - 1; i > 0; i--)
		Result.push_back(GetEventsLogJSONItem(i));

	return "[" + Converter::VectorToString(Result, ",") + "]";
}

/**
 * @brief Determine log item type
 *
 * @param [in] Code log item code
 * @return log item type enum
 */
Log::ItemType Log::GetItemType(uint16_t Code) {
	if (Code <= 0x00FF)
		return SYSTEM;

	if (Code <= 0x1000)
		return ERROR;

	return INFO;
}

/**
 * @brief Correct log items time after device time updated
 */
void Log::CorrectTime() {
	NVS *Memory = new NVS(NVSLogArea);

	for (int i = GetSystemLogCount(Memory) - 1; i > 0; i--) {
		Item Record = GetSystemLogItem(i, Memory);

		if (Time::IsUptime(Record.Time)) {
			Record.Time = Time::Unixtime() - (Time::Uptime() - Record.Time);
			Memory->StringArrayReplace(NVSLogArray, i, Serialize(Record));
		}

		if (Record.Code == 0x0001) break;
	}

	delete Memory;
}

/**
 * @brief Verify is last device start was successefull or not
 *
 * @return if true last device boot was succesefull
 */

bool Log::VerifyLastBoot() {
	NVS *Memory = new NVS(NVSLogArea);

	if (GetSystemLogCount(Memory) == 0)
		return true;

	for (int i = GetSystemLogCount(Memory) - 1; i > 0; i--) {
		Log::Item Record = Log::GetSystemLogItem(i, Memory);

		if (Record.Code == LOG_DEVICE_ON)
			break;

		if (Record.Code == LOG_DEVICE_STARTED || Record.Code == LOG_DEVICE_ROLLBACK)
			return true;
	}

	delete Memory;
	return false;
}

/**
 * @brief Handle http request to device
 *
 * @param [in] Result   Resulted response to the web-server
 * @param [in] Type     GET, POST or DELETE query
 * @param [in] URLParts HTTP path devided by /
 * @param [in] Params   Query params
 */
void Log::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
	// обработка GET запроса - получение данных
	if (Type == QueryType::GET) {

		if (URLParts.size() == 0)
			Result.Body = "{\"System\" : " + GetSystemLogJSON() + ", \"Events\" : " + GetEventsLogJSON() + "}";

		if (URLParts.size() == 1) {
			if (URLParts[0] == "system")
				Result.Body = GetSystemLogJSON();

			if (URLParts[0] == "events")
				Result.Body = GetEventsLogJSON();
		}
	}
}

/**
 * @brief Serialize   log item to JSON string
 * @param [in] Item   log item
 * @return Serialized JSON string items count
 */
string Log::Serialize(Log::Item Item) {
	JSON JSONObject;

	JSONObject.SetItem("Code", Converter::ToHexString(Item.Code, ((GetItemType(Item.Code) == SYSTEM) ? 2 : 4)));
	JSONObject.SetItem("Time", Converter::ToHexString(Item.Time, 8));

	if (GetItemType(Item.Code) != SYSTEM)
		JSONObject.SetItem("Data", Converter::ToHexString(Item.Data, 8));

	return JSONObject.ToString();
}

/**
 * @brief Deserialize log item from JSON string
 * @param [in] JSONString JSON string to deserialize
 * @return log item from JSON string
 */
Log::Item Log::Deserialize(string JSONString) {
	JSON JSONObject(JSONString);

	Log::Item Record;
	Record.Code = Converter::UintFromHexString<uint16_t>(JSONObject.GetItem("code"));
	Record.Data = Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("data"));
	Record.Time = Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("time"));

	return Record;
}
