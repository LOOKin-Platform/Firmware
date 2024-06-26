/*
*    Log.cpp
*    Class to handle API /Log
*
*/

#include "Log.h"

static char tag[] = "Log";

vector<Log::Item> 	Log::Items = vector<Log::Item>();
esp_timer_handle_t	Log::Indicator_t::IndicatorTimer = NULL;
float 				Log::Indicator_t::CurrentBrightness = 0.03;
WS2812*				Log::ws2812 = NULL;

Log::Log() { }

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
		NVS *Memory = new NVS(NVSLogArea);
		pair<void *, size_t> DataFromNVS = Memory->GetBlob(NVSLogArray);

		uint64_t LogBlob[Settings.Log.SystemLogSize] = {0};
		memcpy(LogBlob, DataFromNVS.first, DataFromNVS.second);

		free(DataFromNVS.first);
		//if (DataFromNVS.first != NULL) free(DataFromNVS.first);

		// shift left
		for (int i = Settings.Log.SystemLogSize; i > 0; i--)
			LogBlob[i] = LogBlob[i-1];

		//add new item at start
		LogBlob[0] = Record.Uint64Data;

		Memory->SetBlob(NVSLogArray, LogBlob, Settings.Log.SystemLogSize * sizeof(uint64_t));
		Memory->Commit();
		delete Memory;
	}
	else
	{ // событие, не требующее хранение в NVS
		Items.push_back(Record);

		if (Items.size() >= Settings.Log.EventsLogSize)
			Items.erase(Items.begin());

		if (GetItemType(Code) == ERROR)
			ESP_LOGE(tag, "Error code %04X(%lX)", Code, Data);

		if (GetItemType(Code) == INFO)
			ESP_LOGE(tag, "Info code %04X(%lX)", Code, Data);
	}

	Indicator_t::Display(Code);
}

/**
 * @brief Get system log data
 *
 * @param [in] Index Index of record
 * @return log item with given index.
 */
vector<Log::Item> Log::GetSystemLog() {
	NVS *Memory = new NVS(NVSLogArea);
	vector<Item> Result = vector<Item>();

	pair<void *, size_t> DataFromNVS = Memory->GetBlob(NVSLogArray);

	uint64_t LogBlob[Settings.Log.SystemLogSize] = {0};
	memcpy(LogBlob, DataFromNVS.first, DataFromNVS.second);
	//if (DataFromNVS.first != NULL) free(DataFromNVS.first);

	for (int i=0; i < (DataFromNVS.second / sizeof(uint64_t)); i++) {
		Item ItemToAdd;
		ItemToAdd.Uint64Data = LogBlob[i];
		Result.push_back(ItemToAdd);
	}

	delete Memory;
	return Result;
}

/**
 * @brief Get JSON string with all exists system log records
 * @return JSON string contains log items.
 */
string Log::GetSystemLogJSON() {
	vector<string> Result = vector<string>();

	for (auto& LogItem : GetSystemLog())
	{
		JSON JSONObject;
		JSONObject.SetItem("Code", Converter::ToHexString(LogItem.Code, 4));
		JSONObject.SetItem("Data", Converter::ToHexString(LogItem.Data, 8));
		JSONObject.SetItem("Time", Converter::ToHexString(LogItem.Time, 8));
		Result.push_back(JSONObject.ToString());
	}

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
	JSONObject.SetItem("Data", Converter::ToHexString(Items.at(Index).Data, 4));
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

	pair<void *, size_t> DataFromNVS = Memory->GetBlob(NVSLogArray);

	uint64_t LogBlob[Settings.Log.SystemLogSize] = {0};
	memcpy(LogBlob, DataFromNVS.first, DataFromNVS.second);
	//if (DataFromNVS.first != NULL) free(DataFromNVS.first);

	free(DataFromNVS.first);

	for (int i=0; i < (DataFromNVS.second / sizeof(uint64_t)); i++) {
		Item ItemToModify;
		ItemToModify.Uint64Data = LogBlob[i];
		ItemToModify.Time = Time::Unixtime() - (Time::Uptime() - ItemToModify.Time);
		LogBlob[i] = ItemToModify.Uint64Data;

		if (ItemToModify.Code == 0x0001) break;
	}

	Memory->SetBlob(NVSLogArray, LogBlob, Settings.Log.SystemLogSize * sizeof(uint64_t));
	Memory->Commit();

	delete Memory;
}

/**
 * @brief Verify is last device start was successefull or not
 *
 * @return if true last device boot was succesefull
 */
bool Log::VerifyLastBoot() {
	NVS Memory(NVSLogArea);
	vector<Item> Result = vector<Item>();

	pair<void *, size_t> DataFromNVS = Memory.GetBlob(NVSLogArray);

	uint64_t LogBlob[Settings.Log.SystemLogSize] = {0};
	memcpy(LogBlob, DataFromNVS.first, DataFromNVS.second);
	//if (DataFromNVS.first != NULL) free(DataFromNVS.first);

	if (DataFromNVS.second == 0) return true;

	for (int i=0; i < (DataFromNVS.second / sizeof(uint64_t)); i++) {
		Item ItemToCheck;
		ItemToCheck.Uint64Data = LogBlob[i];

		if (ItemToCheck.Code == Events::System::DeviceOn)
			break;

		if (ItemToCheck.Code == Events::System::DeviceStarted || ItemToCheck.Code == Events::System::DeviceRollback)
			return true;
	}

	return false;
}

/**
 * @brief Use indicator to display code
 *
 * @param [in] LogItem Code of code to display
 */
void Log::Indicator_t::Display(uint16_t LogItem) {

	switch (LogItem) {
		case Events::System		::DeviceOn			: Execute(0		, 255	, 255	, BLINKING	, 10);	break;
		case Events::WiFi		::APStart			: Execute(255	, 255	, 0		, CONST		, 4);	break;
		case Events::WiFi		::STAConnecting		: Execute(0		, 255	, 0		, BLINKING	, 0);	break;
		case Events::WiFi		::STAGotIP			: Execute(0		, 255	, 0		, CONST		, 4);	break;
		case Events::System		::OTAStarted		: Execute(230	, 57 	, 155	, BLINKING	, 300);	break;
		case Events::System		::OTAFailed			:
		case Events::System		::OTAVerifyFailed	: Execute(0		, 0 	, 0		, CONST		, 2);	break;
		case Events::System		::FirstDeviceInit	: Execute(0xEA	, 0x55	, 0x15	, BLINKING	, 300); break;
		case Events::System		::HardResetIntied	: Execute(0x8B	, 2		, 2		, BLINKING	, 300); break;

		case Events::System		::PowerManageOn		: Execute(255	, 239	, 213 	, CONST		, 3);	break;

		case Events::Commands	::IRExecuted		:
		case Events::Sensors 	::IRReceived		: Execute(60, 0, 0, STROBE, 2);	break;

		case Events::Commands	::BLEFailed			:
		case Events::Commands	::BLEExecuted		: Execute(0, 0,255, STROBE, 2); break;

		case Events::Misc		::HomeKitIdentify	: Execute(255	, 255	, 0		, BLINKING	, 5);	break;

		default: break;
	}
}

/**
 * @brief Low-level function to switch on Indicator
 *
 * @param [in] Red		Red color brightness
 * @param [in] Green	Green color brightness
 * @param [in] Blue		Blue color brightness
 * @param [in] Blinking	Indicator
 * @param [in] Duration	Duration of signal
 *
 */

uint8_t	Log::Indicator_t::tRed 		= 0;
uint8_t	Log::Indicator_t::tGreen	= 0;
uint8_t	Log::Indicator_t::tBlue		= 0;

bool 		Log::Indicator_t::IsInited 	= false;
uint32_t	Log::Indicator_t::tDuration = 0;
uint32_t	Log::Indicator_t::tExpired	= 0;
Log::Indicator_t::MODE
			Log::Indicator_t::tBlinking = CONST;

void Log::Indicator_t::Execute(uint8_t Red, uint8_t Green, uint8_t Blue, MODE Blinking, uint16_t Duration) {
	Settings_t::GPIOData_t::Indicator_t GPIOSettings = Settings.GPIOData.GetCurrent().Indicator;

#if CONFIG_IDF_TARGET_ESP32C6
	return;
#endif

	if (!IsInited)
	{
		if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGB || GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGBFrequencyControl)
		{

#if CONFIG_IDF_TARGET_ESP32
			if (GPIOSettings.Red.GPIO 	!= GPIO_NUM_0) 	
				GPIO::SetupPWM(GPIOSettings.Red.GPIO, GPIOSettings.Red.Channel, 1000, LEDC_TIMER_8_BIT, LEDC_TIMER_MAX, LEDC_USE_REF_TICK, (GPIOSettings.Red.IsInverted) ? 255 : 0);
			if (GPIOSettings.Green.GPIO != GPIO_NUM_0) 	
				GPIO::SetupPWM(GPIOSettings.Green.GPIO, GPIOSettings.Green.Channel, 1000, LEDC_TIMER_8_BIT, LEDC_TIMER_MAX, LEDC_USE_REF_TICK, (GPIOSettings.Green.IsInverted) ? 255 : 0);
			if (GPIOSettings.Blue.GPIO 	!= GPIO_NUM_0) 	
				GPIO::SetupPWM(GPIOSettings.Blue.GPIO, GPIOSettings.Blue.Channel, 1000, LEDC_TIMER_8_BIT, LEDC_TIMER_MAX, LEDC_USE_REF_TICK, (GPIOSettings.Blue.IsInverted) ? 255 : 0);
#elif CONFIG_IDF_TARGET_ESP32C6
			if (GPIOSettings.Red.GPIO 	!= GPIO_NUM_0) 	
				GPIO::SetupPWM(GPIOSettings.Red.GPIO, GPIOSettings.Red.Channel, 1000, LEDC_TIMER_8_BIT, LEDC_TIMER_MAX, LEDC_USE_RTC8M_CLK, (GPIOSettings.Red.IsInverted) ? 255 : 0);
			if (GPIOSettings.Green.GPIO != GPIO_NUM_0) 	
				GPIO::SetupPWM(GPIOSettings.Green.GPIO, GPIOSettings.Green.Channel, 1000, LEDC_TIMER_8_BIT, LEDC_TIMER_MAX, LEDC_USE_RTC8M_CLK, (GPIOSettings.Green.IsInverted) ? 255 : 0);
			if (GPIOSettings.Blue.GPIO 	!= GPIO_NUM_0) 	
				GPIO::SetupPWM(GPIOSettings.Blue.GPIO, GPIOSettings.Blue.Channel, 1000, LEDC_TIMER_8_BIT, LEDC_TIMER_MAX, LEDC_USE_RTC8M_CLK, (GPIOSettings.Blue.IsInverted) ? 255 : 0);
#endif

			if (GPIOSettings.Red.GPIO != GPIO_NUM_0 || GPIOSettings.Green.GPIO != GPIO_NUM_0 || GPIOSettings.Blue.GPIO != GPIO_NUM_0)
				GPIO::PWMFadeInstallFunction();

#if CONFIG_IDF_TARGET_ESP32
			if (GPIOSettings.Red.GPIO 	!= GPIO_NUM_0) 	
				ledc_stop(LEDC_HIGH_SPEED_MODE, GPIOSettings.Red.Channel	, (GPIOSettings.Red.IsInverted) 	? 255 : 0);
			if (GPIOSettings.Green.GPIO != GPIO_NUM_0) 	
				ledc_stop(LEDC_HIGH_SPEED_MODE, GPIOSettings.Green.Channel	, (GPIOSettings.Green.IsInverted)	? 255 : 0);
			if (GPIOSettings.Blue.GPIO 	!= GPIO_NUM_0) 	
				ledc_stop(LEDC_HIGH_SPEED_MODE, GPIOSettings.Blue.Channel	, (GPIOSettings.Blue.IsInverted) 	? 255 : 0);
#elif CONFIG_IDF_TARGET_ESP32C6
			if (GPIOSettings.Red.GPIO 	!= GPIO_NUM_0) 	
				ledc_stop(LEDC_LOW_SPEED_MODE, GPIOSettings.Red.Channel		, (GPIOSettings.Red.IsInverted) 	? 255 : 0);
			if (GPIOSettings.Green.GPIO != GPIO_NUM_0) 	
				ledc_stop(LEDC_LOW_SPEED_MODE, GPIOSettings.Green.Channel	, (GPIOSettings.Green.IsInverted)	? 255 : 0);
			if (GPIOSettings.Blue.GPIO 	!= GPIO_NUM_0) 	
				ledc_stop(LEDC_LOW_SPEED_MODE, GPIOSettings.Blue.Channel	, (GPIOSettings.Blue.IsInverted) 	? 255 : 0);
#endif
		}
		else if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::ws2812)
		{
			if (ws2812 == NULL)
				ws2812 = new WS2812(GPIOSettings.Alternative.GPIO, 12);
		}

		const esp_timer_create_args_t TimerArgs = {
				.callback 			= &IndicatorCallback,
				.arg 				= NULL,
				.dispatch_method 	= ESP_TIMER_TASK,
				.name				= "IndicatorTimer",
				.skip_unhandled_events = false
		};

		::esp_timer_create(&TimerArgs, &IndicatorTimer);

		if (GPIOSettings.SwitchLedOn.GPIO != GPIO_NUM_0) {
			GPIO::Setup(GPIOSettings.SwitchLedOn.GPIO);
		}

		NVS Memory(NVSLogArea);
		if (Memory.IsKeyExists(NVSBrightness))
		{
			uint8_t Value = Memory.GetInt8Bit(NVSBrightness);
			if (Value > 100) Value = 100;
			Log::Indicator_t::SetBrightness(Value);
		}

		IsInited = true;
	}

	if (CurrentBrightness == 0)
		return;

	if (GPIOSettings.SwitchLedOn.GPIO != GPIO_NUM_0)
		GPIO::Write(GPIOSettings.SwitchLedOn.GPIO, true);

	tRed 		= (uint8_t)round((Red 	* CurrentBrightness) * ((float)GPIOSettings.Red.MaxDuty 	/ 255));
	tGreen 		= (uint8_t)round((Green	* CurrentBrightness) * ((float)GPIOSettings.Green.MaxDuty 	/ 255));
	tBlue 		= (uint8_t)round((Blue 	* CurrentBrightness) * ((float)GPIOSettings.Blue.MaxDuty 	/ 255));

	if (GPIOSettings.Red.IsInverted) 	tRed	= 255 - tRed;	
	if (GPIOSettings.Green.IsInverted) 	tGreen 	= 255 - tGreen;	
	if (GPIOSettings.Blue.IsInverted) 	tBlue 	= 255 - tBlue;

	tDuration	= Duration * 1000000;
	tExpired	= 0;
	tBlinking	= Blinking;

	if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGB || GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGBFrequencyControl)
	{
		if (GPIOSettings.Red.GPIO 	!= GPIO_NUM_0) 	GPIO::PWMFadeTo(GPIOSettings.Red	, tRed	, 	0);
		if (GPIOSettings.Green.GPIO != GPIO_NUM_0) 	GPIO::PWMFadeTo(GPIOSettings.Green	, tGreen, 	0);
		if (GPIOSettings.Blue.GPIO 	!= GPIO_NUM_0) 	GPIO::PWMFadeTo(GPIOSettings.Blue 	, tBlue	, 	0);

#if CONFIG_IDF_TARGET_ESP32
		if (GPIOSettings.Red.IsInverted) 	ledc_stop(LEDC_HIGH_SPEED_MODE, GPIOSettings.Red.Channel	,  255);
		if (GPIOSettings.Green.IsInverted)	ledc_stop(LEDC_HIGH_SPEED_MODE, GPIOSettings.Green.Channel	,  255);
		if (GPIOSettings.Blue.IsInverted)	ledc_stop(LEDC_HIGH_SPEED_MODE, GPIOSettings.Blue.Channel	,  255);
#elif CONFIG_IDF_TARGET_ESP32C6
		if (GPIOSettings.Red.IsInverted) 	ledc_stop(LEDC_LOW_SPEED_MODE, GPIOSettings.Red.Channel	,  255);
		if (GPIOSettings.Green.IsInverted)	ledc_stop(LEDC_LOW_SPEED_MODE, GPIOSettings.Green.Channel	,  255);
		if (GPIOSettings.Blue.IsInverted)	ledc_stop(LEDC_LOW_SPEED_MODE, GPIOSettings.Blue.Channel	,  255);
#endif
	}
	else if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::ws2812) {
		ws2812->setAllPixels(tRed, tGreen, tBlue);
		ws2812->Show();
	}

	::esp_timer_start_periodic(IndicatorTimer, TIMER_ALARM);
}

/**
 * @brief Hardware timer for indicator const light handler
 */
void Log::Indicator_t::IndicatorCallback(void *Param) {
	Settings_t::GPIOData_t::Indicator_t GPIOSettings = Settings.GPIOData.GetCurrent().Indicator;

	if (tBlinking == NONE) {
		::esp_timer_stop(IndicatorTimer);

		if (GPIOSettings.SwitchLedOn.GPIO != GPIO_NUM_0)
			GPIO::Write(GPIOSettings.SwitchLedOn.GPIO, false);
		return;
	}

	tExpired += TIMER_ALARM;

	bool IsLighted = false;

	if (tDuration > 0 && tExpired > tDuration)
		tBlinking = NONE;

	if (tBlinking == BLINKING || tBlinking == STROBE) {
		uint8_t Divider = (tBlinking == BLINKING) ? BLINKING_DIVIDER : STROBE_DIVIDER;

		if (tExpired % (Divider * TIMER_ALARM) != 0)
			return;

		if (tDuration == 0 && tExpired >= (Divider * TIMER_ALARM))
			tExpired = 0;

		if (tBlinking == STROBE && (tExpired / TIMER_ALARM) >= ((tDuration / 1000000)*2 )) {
			tExpired = 0;
			tBlinking = NONE;
		}

		if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGB || GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGBFrequencyControl) {
			uint32_t RedValue 	= GPIO::PWMValue(GPIOSettings.Red.Channel);
			uint32_t GreenValue = GPIO::PWMValue(GPIOSettings.Green.Channel);
			uint32_t BlueValue 	= GPIO::PWMValue(GPIOSettings.Blue.Channel);
			
			if (GPIOSettings.Red.IsInverted) 	RedValue	= 255 - RedValue;	
			if (GPIOSettings.Green.IsInverted) 	GreenValue 	= 255 - GreenValue;	
			if (GPIOSettings.Blue.IsInverted) 	BlueValue 	= 255 - BlueValue;

			IsLighted =!((RedValue + GreenValue + BlueValue) == 0);
		}
		else if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::ws2812) {
			IsLighted = ws2812->GetIsLighted();
		}
	}

	if ((tDuration > 0 && tExpired > tDuration) || (tBlinking == NONE))
		IsLighted = true;

	if (tBlinking == NONE)
		tRed = tGreen = tBlue = 0;

	if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGB || GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::RGBFrequencyControl)
	{
		uint8_t RedBlank 	= (GPIOSettings.Red.IsInverted) 	? 255 : 0;
		uint8_t GreenBlank 	= (GPIOSettings.Green.IsInverted) 	? 255 : 0;
		uint8_t BlueBlank 	= (GPIOSettings.Blue.IsInverted) 	? 255 : 0;

#if CONFIG_IDF_TARGET_ESP32
		GPIO::PWMFadeTo(GPIOSettings.Red, (IsLighted) ? RedBlank : tRed, 	0);
		if (IsLighted && GPIOSettings.Green.IsInverted) ledc_stop(LEDC_HIGH_SPEED_MODE	, GPIOSettings.Red.Channel	,  255);

		GPIO::PWMFadeTo(GPIOSettings.Green, (IsLighted) ? GreenBlank : tGreen, 	0);
		if (IsLighted && GPIOSettings.Green.IsInverted) ledc_stop(LEDC_HIGH_SPEED_MODE	, GPIOSettings.Green.Channel,  255);

		GPIO::PWMFadeTo(GPIOSettings.Blue, (IsLighted) ? BlueBlank : tBlue, 	0);
		if (IsLighted && GPIOSettings.Blue.IsInverted) ledc_stop(LEDC_HIGH_SPEED_MODE	, GPIOSettings.Blue.Channel	,  255);
#elif CONFIG_IDF_TARGET_ESP32C6
		GPIO::PWMFadeTo(GPIOSettings.Red, (IsLighted) ? RedBlank : tRed, 	0);
		if (IsLighted && GPIOSettings.Green.IsInverted) ledc_stop(LEDC_LOW_SPEED_MODE	, GPIOSettings.Red.Channel	,  255);

		GPIO::PWMFadeTo(GPIOSettings.Green, (IsLighted) ? GreenBlank : tGreen, 	0);
		if (IsLighted && GPIOSettings.Green.IsInverted) ledc_stop(LEDC_LOW_SPEED_MODE	, GPIOSettings.Green.Channel,  255);

		GPIO::PWMFadeTo(GPIOSettings.Blue, (IsLighted) ? BlueBlank : tBlue, 	0);
		if (IsLighted && GPIOSettings.Blue.IsInverted) ledc_stop(LEDC_LOW_SPEED_MODE	, GPIOSettings.Blue.Channel	,  255);
#endif
	}
	else if (GPIOSettings.Type == Settings_t::GPIOData_t::Indicator_t::ws2812) 
	{
		if (!IsLighted)
		{
			ws2812->setAllPixels(tRed, tGreen, tBlue);
			ws2812->Show();
		}
		else
			ws2812->Clear();
	}
}

/**
 * @brief Handle http request to device
 *
 * @param [in] Result   Resulted response to the web-server
 * @param [in] Type     GET, POST or DELETE query
 * @param [in] URLParts HTTP path devided by /
 * @param [in] Params   Query params
 */
void Log::HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) {
	// обработка GET запроса - получение данных
	if (Query.Type == QueryType::GET) {
		if (Query.GetURLPartsCount() == 1)
			Result.Body = "{\"System\" : " + GetSystemLogJSON() + ", \"Events\" : " + GetEventsLogJSON() + "}";

		if (Query.GetURLPartsCount() == 2) {
			if (Query.CheckURLPart("system", 1))
				Result.Body = GetSystemLogJSON();

			if (Query.CheckURLPart("events", 1))
				Result.Body = GetEventsLogJSON();

			if (Query.CheckURLPart("settings", 1)) {
				JSON JSONObject;

				JSONObject.SetFloatItem("IndicatorBrightness", Log::Indicator_t::GetBrightness());

				Result.Body = JSONObject.ToString();
			}
		}
	}

	if (Query.Type == QueryType::PUT) {
		if (Query.CheckURLPart("settings", 1)) {
			JSON JSONObject(Query.GetBody());

			if (JSONObject.IsItemNumber("IndicatorBrightness"))
			{
				int Value = JSONObject.GetIntItem("IndicatorBrightness");
				if (Value < 0) Value = 0;
				if (Value > 100) Value = 100;

				Log::Indicator_t::SetBrightness((uint8_t)Value);
				Log::Indicator_t::Execute(0, 255, 255, Log::Indicator_t::MODE::CONST, 3);

				Result.SetSuccess();
			}
			else
				Result.SetInvalid();
		}
	}
}

uint8_t Log::Indicator_t::GetBrightness() {
	return ceil(CurrentBrightness*BrightnessDelimeter);
}


void Log::Indicator_t::SetBrightness(uint8_t Percent) {
	NVS Memory(NVSLogArea);
	Memory.SetInt8Bit(NVSBrightness, Percent);
	Memory.Commit();

	CurrentBrightness = ((float)Percent) / BrightnessDelimeter;
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
