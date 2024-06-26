/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/

#include "CommandIR.h"
#include "CommandBLE.h"

#include "Storage.h"
#include "LocalMQTT.h"

extern Storage_t Storage;

CommandIR_t::CommandIR_t() {
	ID          				= 0x07;
	Name        				= "IR";

	Events["nec1"]				= 0x01;
	Events["sony"]				= 0x03;
	Events["necx"]				= 0x04;
	Events["panasonic"]			= 0x05;
	Events["samsung36"]			= 0x06;
	Events["rc5"]				= 0x07;
	Events["rc6"]				= 0x08;

	//Events["mce"]				= 0x0A;

	/*
	Events["daikin"]		= 0x08;
	Events["mitsubishi-ac"] = 0x09;
	Events["gree"] 			= 0x0A;
	Events["haier-ac"] 		= 0x0B;
	*/

	Events["aiwa"]				= 0x14;

	Events["repeat"]			= 0xED;

	Events["saved"]				= 0xEE;
	Events["ac"]				= 0xEF;
	Events["prontohex"]			= 0xF0;
	Events["prontohex-blocked"]	= 0xF1;
	Events["prontohex-repeated"]= 0xF2;

	Events["pause"]				= 0xFD;
	Events["localremote"]		= 0xFE;
	Events["raw"]				= 0xFF;

	const esp_timer_create_args_t TimerArgs = {
		.callback 			= &ProntoHexBlockedCallback,
		.arg 				= NULL,
		.dispatch_method 	= ESP_TIMER_TASK,
		.name				= "ProntoHexBlockedTimer",
		.skip_unhandled_events = false
	};

	::esp_timer_create(&TimerArgs, &ProntoHexBlockedTimer);

	ACCode.reserve(800);

	NVS Memory(CommandIRArea);
	Memory.EraseStartedWith(CommandIRTXQueuePrefix);
	Memory.Commit();

	CommandIRTXQueue = FreeRTOS::Queue::Create((uint16_t)Settings.CommandsConfig.IR.SendTaskQueueCount2Gen, sizeof( uint32_t ));

	if (CommandIRTXHandle == NULL) {
		CommandIRTXHandle = FreeRTOS::StartTask(CommandIRTXTask, "RMTTXTask", NULL, 6144, Settings.CommandsConfig.IR.SendTaskPriority);
		//vTaskStartScheduler();
	}
}

void CommandIR_t::InitSettings() {
	NVS Memory(NVSCommandsIRArea);

	if (Memory.IsKeyExists(NVSSeqRepeatCounter))
	{
		SequenceRepeatCounter = Memory.GetInt8Bit(NVSSeqRepeatCounter);
		if (SequenceRepeatCounter < 1)
			SequenceRepeatCounter = 1;
	}
}

string CommandIR_t::GetSettings() {
	return "{\"SequenceRepeatCounter\": " +
			Converter::ToString<uint8_t>(SequenceRepeatCounter)
			+ "}";
}

void CommandIR_t::SetSettings(WebServer_t::Response &Result, Query_t &Query) {
	JSON JSONItem(Query.GetBody());

	if (JSONItem.GetKeys().size() == 0)
	{
		Result.SetInvalid();
		return;
	}

	bool IsChanged = false;

	NVS Memory(NVSCommandsIRArea);

	if (JSONItem.IsItemExists("SequenceRepeatCounter") && JSONItem.IsItemNumber("SequenceRepeatCounter"))
	{
		SequenceRepeatCounter = JSONItem.GetIntItem("SequenceRepeatCounter");
		Memory.SetInt8Bit(NVSSeqRepeatCounter, SequenceRepeatCounter);
		IsChanged = true;
	}

	if (IsChanged)
	{
		Memory.Commit();
		Result.SetSuccess();
	}
	else
		Result.SetInvalid();
}

bool CommandIR_t::Execute(uint8_t EventCode, const char* StringOperand) {
	uint16_t Misc 		= 0x0;
	uint32_t Operand 	= 0x0;

	if (strlen(StringOperand) > 8)
	{
		string ConverterOperand = (strlen(StringOperand) < 20) ? string(StringOperand) : "";

		while (ConverterOperand.size() < 12)
			ConverterOperand = "0" + ConverterOperand;

		if (ConverterOperand.size() > 12)
			ConverterOperand = ConverterOperand.substr(ConverterOperand.size() - 12);

		Operand = Converter::UintFromHexString<uint32_t>(ConverterOperand.substr(0,8));
		Misc 	= Converter::UintFromHexString<uint16_t>(ConverterOperand.substr(8,4));
	}
	else
		Operand = Converter::UintFromHexString<uint32_t>(StringOperand);

	if (EventCode > 0x0 && EventCode < 0xED) { // /commands/ir/nec || /commands/ir/sirc ...
		IRLib IRSignal;
		IRSignal.Protocol 	= EventCode;
		IRSignal.Uint32Data = Operand;
		IRSignal.MiscData	= Misc;

		LastSignal.Protocol = EventCode;
		LastSignal.Data		= Operand;
		LastSignal.Misc		= Misc;
		LastSignal.Frequency= IRSignal.Frequency;

		if (Operand == 0x0 && Misc == 0x0)
			return false;

		vector<int32_t> DataToSend = IRSignal.GetRawDataForSending();

		TXSend(DataToSend, IRSignal.GetProtocolFrequency());
		TriggerStateChange(IRSignal);

		return true;
	}

	if (EventCode == 0xED) { // repeat signal
		uint8_t ProtocolID = Converter::ToUint8(StringOperand);

		if (ProtocolID == 0)
			ProtocolID = LastSignal.Protocol;

		IRLib IRSignal;
		IRSignal.Protocol 	= ProtocolID;
		IRSignal.Uint32Data = LastSignal.Data;
		IRSignal.MiscData	= LastSignal.Misc;

		vector<int32_t> RepeatedSignal = IRSignal.GetRawRepeatSignalForSending();
		if (RepeatedSignal.size() == 0)
			return false;

		TXSend(RepeatedSignal, IRSignal.GetProtocolFrequency());

		return true;
	}

	if (EventCode == 0xEE) { // signal from memory
		uint16_t StorageItemID = Converter::ToUint16(StringOperand);

		uint32_t DataSize	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

		if (StorageItemID <= DataSize / Settings.Storage.Data.ItemSize) {
			Storage_t::Item Item = Storage.Read(StorageItemID);

			Sensor_t* SensorIR = Sensor_t::GetSensorByID(ID + 0x80);
			if (SensorIR == nullptr) {
				ESP_LOGE("CommandIR","Can't get IR sensor to decode message from memory");
				return false;
			}

			map<string,string> DecodedValue = SensorIR->StorageDecode(Item.DataToString());
			if (DecodedValue.count("Signal") > 0) {
				IRLib IRSignal(DecodedValue["Signal"]);

				if (DecodedValue.count("Protocol") > 0)
					IRSignal.Protocol = (uint8_t)Converter::ToUint16(DecodedValue["Protocol"]);

				LastSignal.Protocol 	= IRSignal.Protocol;
				LastSignal.Data 		= IRSignal.Uint32Data;
				LastSignal.Misc			= IRSignal.MiscData;

				vector<int32_t> DataToSend = IRSignal.GetRawDataForSending();

				TXSend(DataToSend, IRSignal.Frequency);
				TriggerStateChange(IRSignal);

				return true;
			}
			else {
				ESP_LOGE("CommandIR","Can't find Data in memory");
				return false;
			}
		}
		else {
			return false;
		}
	}

	if (EventCode == 0xEF) { // AC by codeset
		ACOperand ACData(Operand);

		//ESP_LOGI("ACOperand", "Mode %s", Converter::ToString<uint8_t>(ACData.Mode).c_str());
		//ESP_LOGI("ACOperand", "Temperature %s", Converter::ToString<uint8_t>(ACData.Temperature).c_str());
		//ESP_LOGI("ACOperand", "FanMode %s", Converter::ToString<uint8_t>(ACData.FanMode).c_str());
		//ESP_LOGI("ACOperand", "SwingMode %s", Converter::ToString<uint8_t>(ACData.SwingMode).c_str());

		if (ACData.Codeset == 0) {
			ESP_LOGE("CommandIR","Can't parse AC codeset");
			return false;
		}

		/// FFF0  	- switch on
		/// FFF1	- swing
		/// FFF2	- swing
		pair<uint16_t, uint16_t>  Statuses = make_pair(0xFFFF, 0xFFFF);

		if (Settings.eFuse.Type == Settings.Devices.Remote)
			Statuses = ((DataRemote_t*)Data)->GetStatusPair(0xEF, ACData.GetCodesetHEX());

		uint16_t CurrentStatus	= Statuses.first;
		uint16_t LastStatus		= Statuses.second;

		bool  IsOnSeparate= ACOperand::IsOnSeparateForCodeset(ACData.GetCodesetHEX());

		bool ShouldOn = false;

		if (ACData.ToUint16() == 0xFFF0 || (IsOnSeparate && CurrentStatus < 0x1000 && LastStatus > 0x0FFF)) // if switch on signal received
			ShouldOn = true;

		if (ShouldOn && LastStatus == 0)
			LastStatus = 0x2700;

		if (ShouldOn && (CurrentStatus < 0x1000 || CurrentStatus == 0xFFFF)) {
			HTTPClient::Query(	Settings.ServerUrls.GetACCode + "?operand=" + ACData.GetCodesetString() + "FFF0",
								QueryType::POST, false, true,
								&ACReadStarted,
								&ACReadBody,
								&ACReadFinished,
								&ACReadAborted,
								Settings.ServerUrls.ACtoken);

			if (LastStatus == 0xFFFF)
				return true;

			if (LastStatus != 0xFFFF && CurrentStatus > 0x0FFF)
				return true;

			if (ACData.ToUint16() == 0xFFF0 && LastStatus != 0xFFFF)
				ACData.SetStatus(LastStatus);
		}

		HTTPClient::Query(	Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery(),
							QueryType::POST, false, true,
							&ACReadStarted,
							&ACReadBody,
							&ACReadFinished,
							&ACReadAborted,
							Settings.ServerUrls.ACtoken);

		return true;
	}

	if (EventCode == 0xF0) { // Send in ProntoHex
		if (::strcmp(StringOperand, "0") == 0 || InOperation)
			return false;

		IRLib IRSignal(StringOperand);

		LastSignal.Protocol	= IRSignal.Protocol;
		LastSignal.Data 	= IRSignal.Uint32Data;
		LastSignal.Misc		= IRSignal.MiscData;
		LastSignal.Frequency= IRSignal.Frequency;

		vector<int32_t> DataToSend = IRSignal.GetRawDataForSending();

		string Symbol3;
		Symbol3.push_back(StringOperand[2]);
		uint8_t AdditionalRepeat = Converter::UintFromHexString<uint8_t>(Symbol3);

		bool TXSendResult = false;
		TXSendResult = TXSend(DataToSend, IRSignal.Frequency, false);

		if (AdditionalRepeat > 0 && IRSignal.RepeatPause > 0)
		{
			int32_t Pause = IRSignal.RepeatPause - abs(DataToSend.back());

			if (Pause < 0)
				Pause = 0;
			else
				Pause = round(Pause / 1000);

			for (int i =0;i < AdditionalRepeat ; i++)
			{
				TXSendAddPause(Pause);
				TXSend(DataToSend, IRSignal.Frequency, false);
			}
		}

		TriggerStateChange(IRSignal);

		return TXSendResult;
	}

	if (EventCode == 0xF1) { // Blocked ProntoHEX
		if (ProntoHexBlockedBuffer != "")
			return false;

		ProntoHexBlockedBuffer = StringOperand;

		::esp_timer_stop(ProntoHexBlockedTimer);
		::esp_timer_start_once(ProntoHexBlockedTimer, Settings.CommandsConfig.IR.ProntoHexBlockedDelayU);

		return Execute(0xF0, StringOperand);
	}

	if (EventCode == 0xF2) { // repeated ProntoHEX with defined repeat counter in settings
		bool IsExecuted = Execute(0xF0, StringOperand);

		for (int i=0; i < SequenceRepeatCounter - 1; i++) {
			IRLib RepeatSignal;
			RepeatSignal.Protocol 	= LastSignal.Protocol;
			RepeatSignal.Uint32Data = LastSignal.Data;
			RepeatSignal.MiscData 	= LastSignal.Misc;
			RepeatSignal.Frequency	= (RepeatSignal.Frequency > 0) ? RepeatSignal.Frequency : 38000;

			vector<int32_t> RepeatRawSignal = RepeatSignal.GetRawRepeatSignalForSending();

			if (RepeatRawSignal.size() > 0)
				TXSend(RepeatRawSignal, RepeatSignal.Frequency);
			else
				Execute(0xF0, StringOperand);
		}

		return IsExecuted;
	}

	if (EventCode == 0xFD) { // add pause (ms) to queue
		if (strlen(StringOperand) > 4)
			return false;

		if (Settings.eFuse.Type != Settings.Devices.Remote)
			return false;

		TXSendAddPause(Converter::ToUint16(string(StringOperand)));

		return true;
	}


	if (EventCode == 0xFE) { // local saved remote in /data
		if (strlen(StringOperand) != 8 || InOperation)
			return false;

		if (Settings.eFuse.Type != Settings.Devices.Remote)
			return false;

		string  SavedRemoteOperand(StringOperand,8);

		string  UUID 		= SavedRemoteOperand.substr(0, 4);

		DataRemote_t::IRDeviceCacheItem_t CacheItem = ((DataRemote_t*)Data)->GetDeviceFromCache(UUID);

		if (CacheItem.DeviceType == 0xEF) { // AC unit
			string ExecuteOperand = Converter::ToHexString(CacheItem.Extra,4) + SavedRemoteOperand.substr(4, 4);
			return Execute(0xEF, ExecuteOperand.c_str());
		}

		uint8_t FunctionID	= Converter::UintFromHexString<uint8_t>(SavedRemoteOperand.substr(4, 2));
		string  Function 	= ((DataRemote_t*)Data)->DevicesHelper.FunctionNameByID(FunctionID);
		uint8_t SignalID 	= Converter::UintFromHexString<uint8_t>( SavedRemoteOperand.substr(6,2));

		string 	FunctionType = ((DataRemote_t*)Data)->GetFunctionType(UUID, Function);

		if (FunctionType != "sequence" && SignalID == 0xFF)
			SignalID = 0x0;

		if (FunctionType == "sequence")
			SignalID = 0xFF;

		if (SignalID != 0xFF)
		{
			DataRemote_t::DataSignal SignalToAdd = ((DataRemote_t*)Data)->LoadFunctionByIndex(UUID, Function, SignalID);

			if (SignalToAdd.Type == DataRemote_t::SignalType::IR) 
			{
				bool Result = TXSendHelper(SignalToAdd.IRSignal, Operand, Misc);

				if (!Result) 
					return false;
			}
			else if (SignalToAdd.Type == DataRemote_t::SignalType::BLE)
			{
				BLESendToQueue(SignalToAdd.BLESignal);
			}
		}
		else
		{
			for (DataRemote_t::DataSignal SignalIterator : ((DataRemote_t*)Data)->LoadAllFunctionSignals(UUID, Function)) {
				if (SignalIterator.Type == DataRemote_t::SignalType::IR) 
				{
					bool Result = TXSendHelper(SignalIterator.IRSignal, Operand, Misc);
					if (!Result) 
						return false;
				}
				else if (SignalIterator.Type == DataRemote_t::SignalType::BLE)
				{
					BLESendToQueue(SignalIterator.BLESignal);
				}

				if (FunctionType == "sequence")
					TXSendAddPause(500);
			}
		}

		((DataRemote_t*)Data)->StatusUpdateForDevice(UUID, FunctionID, SignalID);

		return true;
	}

	if (EventCode == 0xFF) { // Send RAW Format
		if (::strcmp(StringOperand, "0") == 0 || InOperation)
			return false;

		uint16_t Frequency = 38000;

		ACCode.clear();

		char *FrequencyDelimeterPointer = strstr(StringOperand, ";");
		if (FrequencyDelimeterPointer != NULL) {
			size_t FrequencyDelimeterPos =  FrequencyDelimeterPointer - StringOperand;
			Frequency = Converter::ToUint16(string(StringOperand, FrequencyDelimeterPos));
			StringOperand += FrequencyDelimeterPos + 1;
		}

		if (::strstr(StringOperand," ") == NULL && ::strstr(StringOperand,"%20") == NULL)
			return false;

		string 		SignalItem 	= "";
		char		SignalChar[1];

		for (int i = 0; i < strlen(StringOperand); i++) {
			memcpy(SignalChar, StringOperand + i, 1);
			string SignalCharStr(SignalChar,1);

			if ((SignalCharStr == string(" ")) || (SignalCharStr == string("%")))
			{
				if (SignalCharStr == string("%")) i+=2;

				ACCode.push_back(Converter::ToInt32(SignalItem));
				SignalItem = "";
			}
			else
				SignalItem += SignalCharStr;
		}

		ACCode.push_back(Converter::ToInt32(SignalItem));

		IRLib IRSignal(ACCode);

//				IRSignal.RawData.push_back(Converter::ToInt32(SignalItem));
//				IRSignal.ExternFillPostOperations();

		LastSignal.Protocol = IRSignal.Protocol;
		LastSignal.Data 	= IRSignal.Uint32Data;
		LastSignal.Misc		= IRSignal.MiscData;
		LastSignal.Frequency= IRSignal.Frequency;

		vector<int32_t> SignalToSend = IRSignal.GetRawDataForSending();

		TXSend(SignalToSend, Frequency);
		TriggerStateChange(IRSignal);

		return true;
	}

	return false;
}

void CommandIR_t::ProntoHexBlockedCallback(void *Param) {
	ProntoHexBlockedBuffer = "";
}

bool CommandIR_t::TXSendHelper(IRLib &Signal,uint32_t Operand, uint16_t Misc)
{
	LastSignal.Protocol = Signal.Protocol;
	LastSignal.Data		= Signal.Uint32Data;
	LastSignal.Misc		= Signal.MiscData;

	if (Operand == 0x0 && Misc == 0x0)
		return false;

	vector<int32_t> SignalVector = Signal.GetRawDataForSending();

	TXSend(SignalVector, Signal.GetProtocolFrequency());
	return true;
}

void CommandIR_t::BLESendToQueue(string Signal) {
	uint32_t HashID = esp_random();

	CommandIRTXQueueData BLESignal;
	BLESignal.NVSItem		= Settings.CommandsConfig.BLE.DataPrefix + Signal;
	BLESignal.Frequency 	= 0;

	CommandIRTXDataMap[HashID] = BLESignal;

	if (SendToCommandIRQueue(HashID))
		ESP_LOGD("RMT", "Added to queue BLE Signal: %s", Signal.c_str());
}

void CommandIR_t::TXSendAddPause(uint32_t PauseLength) {
	uint32_t HashID = esp_random();

	CommandIRTXQueueData TXPause;
	TXPause.NVSItem		= CommandIRTXPause;
	TXPause.Frequency 	= PauseLength;

	CommandIRTXDataMap[HashID] = TXPause;

	if (SendToCommandIRQueue(HashID))
		ESP_LOGD("RMT", "Added to queue PAUSE with length: %lu", PauseLength);
}

bool CommandIR_t::TXSend(vector<int32_t> &TXItemData, uint16_t Frequency, bool ShouldFree) {
	if (TXItemData.empty())
		return false;

	uint32_t HashID = esp_random();
	string HashIDStr = CommandIRTXQueuePrefix + Converter::ToLower(Converter::ToHexString(HashID,8));

	/*
	while (Memory.CheckBlobExists(HashIDStr)) {
		HashID 		= esp_random();
		HashIDStr 	= CommandIRTXQueuePrefix + Converter::ToLower(Converter::ToHexString(HashID,8));
	}
	*/

	/*
	if (Settings.eFuse.DeviceID < 0x100) {
		string SignalToDebug = "";

		for (auto& SignalPart : TXItemData)
			SignalToDebug = SignalToDebug + " " + Converter::ToString<int32_t>(SignalPart);

		ESP_LOGE("Signal to send", "%s", SignalToDebug.c_str());
	}
	*/

	if (HashIDStr.size() > 15)
		return false;

	NVS *Memory = new NVS(CommandIRArea);
	Memory->SetBlob(HashIDStr, (void*)TXItemData.data(), TXItemData.size()*4);// static_cast<void*>(QueueTXItem.data()), 0);
	Memory->Commit();
	delete Memory;

	//TXItemData.clear();

	CommandIRTXQueueData TXData;
	TXData.NVSItem		= HashIDStr;
	TXData.Frequency 	= Frequency;

	CommandIRTXDataMap[HashID] = TXData;

	bool SendToResult = SendToCommandIRQueue(HashID);

	if (SendToResult) {
		IncrementCounter();
		ESP_LOGD("RMT", "Added to queue ID %s", HashIDStr.c_str());
	}

	return SendToResult;
}

void CommandIR_t::IncrementCounter() {
	CommandIR_t* CommandIR = (CommandIR_t*)Command_t::GetCommandByName("IR");
	if (CommandIR == nullptr) return;

	CommandIR->SendCounter++;
}

bool CommandIR_t::SendToCommandIRQueue(uint32_t &HashID) {
	bool IsQueueFull = false;

	uint8_t QueueSize = (Settings.DeviceGeneration < 2) ? Settings.CommandsConfig.IR.SendTaskQueueCount1Gen : Settings.CommandsConfig.IR.SendTaskQueueCount2Gen;
	IsQueueFull = (QueueSize == FreeRTOS::Queue::Count(CommandIRTXQueue)) ? true : false;

	ESP_LOGE("Count from root", "%d IsFull? %s", FreeRTOS::Queue::Count(CommandIRTXQueue), (IsQueueFull) ? "Yes" : "No");

	//if (FreeRTOS::Queue::Count(CommandIRTXQueue) > 5)
	//	FreeRTOS::SetTaskPriority(CommandIRTXHandle, Settings.CommandsConfig.IR.SendTaskPeakPriority);

	if (!IsQueueFull) {
		FreeRTOS::Queue::Send(CommandIRTXQueue, &HashID, false);

		if (CommandIRTXHandle != NULL)
			xTaskNotify(CommandIRTXHandle, (uint32_t)1, eSetValueWithOverwrite);

		return true;
	}

	return false;
}

void CommandIR_t::CommandIRTXTask(void *TaskData) {
	static uint32_t HashID = 0;
	static uint32_t	LastFrequency = 0;

	const uint16_t 	TicksToWaitFromCommandQueue = 200/portTICK_PERIOD_MS;
	const uint16_t 	TicksToWaitFromSensorQueue 	= 100/portTICK_PERIOD_MS;

	if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0) {
		RMT::SetRXChannel(Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38, RMT_CHANNEL_2, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
		ESP_ERROR_CHECK(::rmt_rx_start(RMT_CHANNEL_2 , 1));
	}

	RingbufHandle_t rb = NULL;
	rmt_get_ringbuf_handle(RMT_CHANNEL_2, &rb);

	while (1)
	{
		uint32_t ulNotifiedValue = 0;
		xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, TicksToWaitFromCommandQueue);

		if (FreeRTOS::Queue::Count(CommandIRTXQueue) == 0 && ulNotifiedValue == 0)
		{
			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 == GPIO_NUM_0)
			{ continue; }

			if (rb == NULL) { continue; }

			size_t SignalSize = 0;
			rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &SignalSize, TicksToWaitFromSensorQueue);

			int offset = 0;

			if (SignalSize)
			{
				SensorIR_t::MessageStart();

				while (offset < (int)SignalSize)
				{
					if (!SensorIR_t::MessageBody(RMT::PrepareBit((bool)(item+offset)->level0, (item+offset)->duration0)))
						break;

					if (!SensorIR_t::MessageBody(RMT::PrepareBit((bool)(item+offset)->level1, (item+offset)->duration1)))
						break;

					offset++;
				}

				SensorIR_t::MessageEnd();
			}

			//after parsing the data, return spaces to ringbuffer.
			if (item != NULL)
				vRingbufferReturnItem(rb, (void*) item);

			continue;
		}
		else
		{
			PowerManagement::AddLock("CommandIRTXTask");
			FreeRTOS::SetTaskPriority(CommandIRTXHandle, Settings.CommandsConfig.IR.SendTaskPeakPriority);

			vector<gpio_num_t> GPIO = Settings.GPIOData.GetCurrent().IR.SenderGPIOs;

			if (Settings.GPIOData.GetCurrent().IR.SenderGPIOExt != GPIO_NUM_0)
				GPIO.push_back(Settings.GPIOData.GetCurrent().IR.SenderGPIOExt);

			if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1) {
				for (auto& PinItem : GPIO)
					gpio_matrix_out(PinItem, 0x100, 0, 0);

				((DataRemote_t *)Data)->ClearChannels(GPIO);
			}

			bool IsRMTTXInited = false;

			static int32_t *ItemsToSend;

			NVS *Memory = new NVS(CommandIRArea);

			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0) {
				ESP_ERROR_CHECK(::rmt_rx_stop(RMT_CHANNEL_2));
				RMT::UnsetRXChannel(RMT_CHANNEL_2);
			}

			while (FreeRTOS::Queue::Receive(CommandIRTXQueue, &HashID, 1000/portTICK_PERIOD_MS)) {
				if (CommandIRTXDataMap.count(HashID) == 0)
					continue;

				if (CommandIRTXDataMap[HashID].NVSItem == CommandIRTXPause)
				{
					ESP_LOGD("RMT TX TASK", "Pause for: %lu ms", CommandIRTXDataMap[HashID].Frequency);
					FreeRTOS::Sleep(CommandIRTXDataMap[HashID].Frequency);
				}
				else if (CommandIRTXDataMap[HashID].NVSItem.size() > 4 && (CommandIRTXDataMap[HashID].NVSItem.rfind(Settings.CommandsConfig.BLE.DataPrefix, 0) == 0)) {
					CommandBLE_t* BLECommand = (CommandBLE_t *)Command_t::GetCommandByName("BLE");

					if (BLECommand != nullptr)
						BLECommand->Execute(0x01, CommandIRTXDataMap[HashID].NVSItem.substr(4).c_str());
				}
				else
				{
					pair<void*, size_t> Item = Memory->GetBlob(CommandIRTXDataMap[HashID].NVSItem);

					if (Item.second == 0)
						continue;

					RMT::TXClear();

					ItemsToSend = NULL;
					ItemsToSend = (int32_t *)Item.first;

					for (uint16_t i = 0; i < Item.second / 4; i++) {
						RMT::TXAddItem(*(ItemsToSend + i));
					}

					if (!IsRMTTXInited) { // Первая отправка
						RMT::SetTXChannel(GPIO, TXChannel, CommandIRTXDataMap[HashID].Frequency);
						IsRMTTXInited = true;
					}
					else if (LastFrequency != CommandIRTXDataMap[HashID].Frequency)
					{
						ESP_LOGE("RMT TX TASK", "FREQ CHANGE");
						RMT::UnsetTXChannel(TXChannel);
						RMT::SetTXChannel(GPIO, TXChannel, CommandIRTXDataMap[HashID].Frequency);
					}

					RMT::TXSend(TXChannel);

					ESP_LOGI("RMT TX TASK", "Sended signal with frequency %lu", CommandIRTXDataMap[HashID].Frequency);

					LastFrequency = CommandIRTXDataMap[HashID].Frequency;

					free(Item.first);

					Log::Add(Log::Events::Commands::IRExecuted);

					Memory->Erase(CommandIRTXDataMap[HashID].NVSItem);
				}

				CommandIRTXDataMap.erase(HashID);
				HashID = 0;

				CommandsIRLastSignalTime = ::Time::UptimeU();
			}

			Memory->Commit();
			delete (Memory);

			RMT::UnsetTXChannel(TXChannel);

			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0) {
				ESP_LOGE("TASK", "SetRXChannel");
				RMT::SetRXChannel(Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38, RMT_CHANNEL_2, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
				ESP_ERROR_CHECK(::rmt_rx_start(RMT_CHANNEL_2 , 1));
				rmt_get_ringbuf_handle(RMT_CHANNEL_2, &rb);
			}

			ESP_LOGD("CommandIRTXTask", "RMT TX Task queue loop finished");

			PowerManagement::ReleaseLock("CommandIRTXTask");
			FreeRTOS::SetTaskPriority(CommandIRTXHandle, Settings.CommandsConfig.IR.SendTaskPriority);
		}
	}

	ESP_LOGE("CommandIRTXTask", "Ended");

	FreeRTOS::DeleteTask();
}

void CommandIR_t::TriggerStateChange(IRLib &Signal) {
	if (CommandIRLastSignalCRC != "")
		if (Signal.GetSignalCRC() == CommandIRLastSignalCRC)
			return;

	if (Signal.RawData.size() == 0)
		return;

	CommandIRLastSignalCRC = Signal.GetSignalCRC();

	if (Settings.eFuse.Type == Settings.Devices.Remote) {
		if (((DataRemote_t*)Data)->SetExternalStatusByIRCommand(Signal))
			CommandIRLastSignalCRC = "";
	}
}

// AC Codeset callbacks

void CommandIR_t::ACReadStarted(const char *IP) {
	RMT::TXClear();

	IRACFrequency = 38000;
	ACCode.clear();
	ACReadPart = "";
}

bool CommandIR_t::ACReadBody(char* Data, int DataLen, const char *IP) {
	char *FreqDelimeterPos = (char*)memchr(Data, ':', DataLen);

	//ESP_LOGE("ACReadBody", "%u %s", DataLen, Data);

	if (FreqDelimeterPos != NULL && (FreqDelimeterPos - Data) < 10) {
		string Frequency(Data, (Data - FreqDelimeterPos - 1));
		ESP_LOGE("Frequency", "%s" ,Frequency.c_str());
		IRACFrequency = Converter::ToUint16(Frequency);
	}

	if (FreqDelimeterPos == NULL)
		FreqDelimeterPos = Data;

	char *LastSpacePos = FreqDelimeterPos;
	for (int i = (FreqDelimeterPos - Data); i < DataLen; i++) {
		char* SpacePos = (char*)memchr(Data + i, ' ', DataLen - i);

		if (SpacePos == NULL)
			break;

		int DigitsGroupLen = SpacePos - LastSpacePos;

		string Item(Data + i, DigitsGroupLen);
		if (ACReadPart != "") {
			Item = ACReadPart + Item;
			ACReadPart = "";
		}

		Converter::Trim(Item);

		ACCode.push_back(Converter::ToInt32(Item));
		//ESP_LOGE("ITEM FROM HTTP", "DigitsGroupLen %i %s", DigitsGroupLen , Item.c_str());
		i += DigitsGroupLen;

		LastSpacePos = SpacePos + 1;
	}

	if (LastSpacePos != NULL && (LastSpacePos - Data) > 0)
	{
		ACReadPart = string(LastSpacePos, DataLen - (LastSpacePos - Data));
		//ESP_LOGE("ReadPart", "%s", ACReadPart.c_str());
	}

	return true;
};

void CommandIR_t::ACReadFinished(const char *IP) {
	if (ACReadPart != "")
	{
		Converter::Trim(ACReadPart);
		ACCode.push_back(Converter::ToInt32(ACReadPart));
		ACReadPart = "";
	}

	if (ACCode.size() > 0)
		if (ACCode.back() != -Settings.SensorsConfig.IR.SignalEndingLen)
			ACCode.push_back(-Settings.SensorsConfig.IR.SignalEndingLen);

	CommandIR_t* CommandIR = (CommandIR_t*)Command_t::GetCommandByName("IR");
	if (CommandIR == nullptr)
		ESP_LOGE("CommandIR","Can't get IR command");
	else
		CommandIR->TXSend(ACCode, IRACFrequency, false);

	ACCode.clear();

	Query_t Query(IP, WebServer_t::QueryTransportType::WebServer);
	map<string,string> Params = Query.GetParams();

	if (Params.count("operand") > 0 && Settings.eFuse.Type == Settings.Devices.Remote)
		if (Params["operand"].size() == 8) {
			uint16_t Codeset = Converter::UintFromHexString<uint16_t>(Params["operand"].substr(0, 4));
			uint16_t Operand = Converter::UintFromHexString<uint16_t>(Params["operand"].substr(4, 4));

			pair<bool, uint16_t> StatusSetResult = ((DataRemote_t *)Data)->SetExternalStatusForAC(Codeset, Operand);

			if (ACOperand::IsOnSeparateForCodeset(Codeset) && Operand == 0xFFF0)
				CommandIR->TXSendAddPause(500);

			if (StatusSetResult.first && LocalMQTT_t::GetStatus() == LocalMQTT_t::CONNECTED) 
			{
				string DeviceID =  ((DataRemote_t*)Data)->GetUUIDByExtraValue(Codeset);

				if (DeviceID != "")
					LocalMQTT_t::SendMessage("{\"UUID\": \""+DeviceID+"\", \"Status\":\"" + Converter::ToHexString(StatusSetResult.second,4) + "\"}", "/ir/localremote/sent");
			}
		}
}

void CommandIR_t::ACReadAborted(const char *IP) {
	ACCode.clear();

	RMT::TXClear();
	ESP_LOGE("CommandIR", "Failed to retrieve ac code from server");
}
