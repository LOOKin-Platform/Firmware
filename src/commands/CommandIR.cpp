/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/
#ifndef COMMANDS_IR
#define COMMANDS_IR

#include <RMT.h>
#include "Sensors.h"
#include "CommandBLE.cpp"

#include "Data.h"

#define CommandIRArea 			"RMT"
#define CommandIRTXQueuePrefix 	"tx_"
#define	CommandIRTXPause		"pause"

extern DataEndpoint_t *Data;

static rmt_channel_t TXChannel = RMT_CHANNEL_2;

static uint16_t					IRACFrequency			= 38000;
static string 					ACReadPart				= "";


static string 					ProntoHexBlockedBuffer 	= "";
static esp_timer_handle_t 		ProntoHexBlockedTimer 	= NULL;

static QueueHandle_t 			CommandIRTXQueue;
static TaskHandle_t 			CommandIRTXHandle		= NULL;

static uint64_t					CommandsIRLastSignalTime= 0;

static uint32_t					CommandIRSendCounter 	= 0;

struct CommandIRTXQueueData {
	string      		NVSItem;
	uint16_t			Frequency;
	vector<gpio_num_t>	TXGPIO;
	rmt_channel_t		TXChannel;
};

static map<uint32_t, CommandIRTXQueueData>
								CommandIRTXDataMap		= map<uint32_t, CommandIRTXQueueData>();

static string			CommandIRLastSignalCRC = "";

static vector<int32_t> 	ACCode = vector<int32_t>();

class CommandIR_t : public Command_t {
	public:
		struct LastSignal_t {
			uint8_t 	Protocol 		= 0;
			uint32_t 	Data			= 0;
			uint16_t 	Misc			= 0;
		};

		CommandIR_t() {
			ID          				= 0x07;
			Name        				= "IR";

			Events["nec1"]				= 0x01;
			Events["sony"]				= 0x03;
			Events["necx"]				= 0x04;
			Events["panasonic"]			= 0x05;
			Events["samsung36"]			= 0x06;

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

		LastSignal_t LastSignal;

		bool Execute(uint8_t EventCode, const char* StringOperand) override {
			uint16_t Misc 		= 0x0;
			uint32_t Operand 	= 0x0;

			if (strlen(StringOperand) > 8)
			{
				string ConverterOperand = (strlen(StringOperand) < 20) ? string(StringOperand) : "";

				while (ConverterOperand.size() < 12)
					ConverterOperand = "0" + ConverterOperand;

				if (ConverterOperand.size() > 12)
					ConverterOperand = ConverterOperand.substr(ConverterOperand.size() - 12);

				Misc 	= Converter::UintFromHexString<uint16_t>(ConverterOperand.substr(0,4));
				Operand = Converter::UintFromHexString<uint32_t>(ConverterOperand.substr(4,8));
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

				int 		OnType 		= 0;
				uint16_t 	OnStatus	= 0;


				if (ACData.ToUint16() == 0xFFF0) {
					OnType = (ACOperand::IsOnSeparateForCodeset(ACData.GetCodesetHEX())) ? 1 : 2;

					if (Settings.eFuse.Type == Settings.Devices.Remote)
						OnStatus = ((DataRemote_t*)Data)->GetLastStatus(0xEF, ACData.GetCodesetHEX());
				}


				if (OnType < 2)
					HTTPClient::Query(	Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery(),
										QueryType::POST, false, true,
										&ACReadStarted,
										&ACReadBody,
										&ACReadFinished,
										&ACReadAborted,
										Settings.ServerUrls.ACtoken);

				// switch on if first switch on action ever
				if (OnType > 0 && OnStatus == 0)
					OnStatus = 0x2700;

				if (OnType > 0 && OnStatus > 0) {
					ACData.SetStatus(OnStatus);

					HTTPClient::Query(	Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery(),
										QueryType::POST, false, true,
										&ACReadStarted,
										&ACReadBody,
										&ACReadFinished,
										&ACReadAborted,
										Settings.ServerUrls.ACtoken);
				}

				return true;
			}

			if (EventCode == 0xF0) { // Send in ProntoHex
				if (::strcmp(StringOperand, "0") == 0 || InOperation)
					return false;

				IRLib IRSignal(StringOperand);

				LastSignal.Protocol	= IRSignal.Protocol;
				LastSignal.Data 	= IRSignal.Uint32Data;
				LastSignal.Misc		= IRSignal.MiscData;

				vector<int32_t> DataToSend = IRSignal.GetRawDataForSending();

				bool TXSendResult = TXSend(DataToSend, IRSignal.Frequency, false);
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

			if (EventCode == 0xFE) { // local saved remote in /data
				if (strlen(StringOperand) != 8 || InOperation)
					return false;

				if (Settings.eFuse.Type != Settings.Devices.Remote)
					return false;

				string  SavedRemoteOperand(StringOperand,8);

				string  UUID 		= SavedRemoteOperand.substr(0, 4);
				uint8_t FunctionID	= Converter::UintFromHexString<uint8_t>(SavedRemoteOperand.substr(4, 2));
				string  Function 	= ((DataRemote_t*)Data)->DevicesHelper.FunctionNameByID(FunctionID);
				uint8_t SignalID 	= Converter::UintFromHexString<uint8_t>( SavedRemoteOperand.substr(6,2));

				string FunctionType = ((DataRemote_t*)Data)->GetFunctionType(UUID, Function);

				if (FunctionType != "sequence" && SignalID == 0xFF)
					SignalID = 0x0;

				if (FunctionType == "sequence")
					SignalID = 0xFF;

				if (SignalID != 0xFF)
				{
					DataRemote_t::DataSignal SignalToAdd = ((DataRemote_t*)Data)->LoadFunctionByIndex(UUID, Function, SignalID);

					if (SignalToAdd.Type == DataRemote_t::SignalType::IR) {
						bool Result = TXSendHelper(SignalToAdd.IRSignal, Operand, Misc);
						if (!Result) return false;
					}
					else if (SignalToAdd.Type == DataRemote_t::SignalType::BLE)
					{
						BLESendToQueue(SignalToAdd.BLESignal);
					}
				}
				else
				{
					for (DataRemote_t::DataSignal SignalIterator : ((DataRemote_t*)Data)->LoadAllFunctionSignals(UUID, Function))
						if (SignalIterator.Type == DataRemote_t::SignalType::IR) {
							bool Result = TXSendHelper(SignalIterator.IRSignal, Operand, Misc);
							if (!Result) return false;
						}
						else if (SignalIterator.Type == DataRemote_t::SignalType::BLE)
						{
							BLESendToQueue(SignalIterator.BLESignal);
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

				vector<int32_t> SignalToSend = IRSignal.GetRawDataForSending();

				TXSend(SignalToSend, Frequency);
				TriggerStateChange(IRSignal);

				return true;
			}

			return false;
		}

		static void ProntoHexBlockedCallback(void *Param) {
			ProntoHexBlockedBuffer = "";
		}

		bool TXSendHelper(IRLib &Signal,uint32_t Operand, uint16_t Misc)
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

		static void BLESendToQueue(string Signal) {
			uint32_t HashID = esp_random();

			CommandIRTXQueueData BLESignal;
			BLESignal.NVSItem		= Settings.CommandsConfig.BLE.DataPrefix + Signal;
			BLESignal.Frequency 	= 0;

			CommandIRTXDataMap[HashID] = BLESignal;

			if (SendToCommandIRQueue(HashID))
				ESP_LOGD("RMT", "Added to queue BLE Signal: %s", Signal.c_str());
		}

		static void TXSendAddPause(uint16_t PauseLength = 500) {
			uint32_t HashID = esp_random();

			CommandIRTXQueueData TXPause;
			TXPause.NVSItem		= CommandIRTXPause;
			TXPause.Frequency 	= PauseLength;

			CommandIRTXDataMap[HashID] = TXPause;

			if (SendToCommandIRQueue(HashID))
				ESP_LOGD("RMT", "Added to queue PAUSE with length: %d", PauseLength);
		}

		static bool TXSend(vector<int32_t> &TXItemData, uint16_t Frequency = 38000, bool ShouldFree = true) {
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
    			CommandIRSendCounter++;
				ESP_LOGD("RMT", "Added to queue ID %s", HashIDStr.c_str());
			}

			return SendToResult;
		}

		static bool SendToCommandIRQueue(uint32_t &HashID) {
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

		static void CommandIRTXTask(void *TaskData) {
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
		        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, TicksToWaitFromCommandQueue );

		        if (FreeRTOS::Queue::Count(CommandIRTXQueue) == 0 && ulNotifiedValue == 0)
		        {
		        	if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 == GPIO_NUM_0)
		        		continue;

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
		        			ESP_LOGD("RMT TX TASK", "Pause for: %d ms", CommandIRTXDataMap[HashID].Frequency);
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

		        			ESP_LOGI("RMT TX TASK", "Sended signal with frequency %u", CommandIRTXDataMap[HashID].Frequency);

		        			LastFrequency = CommandIRTXDataMap[HashID].Frequency;

		        			free(Item.first);

		        			Log::Add(Log::Events::Commands::IRExecuted);

		        			Memory->Erase(CommandIRTXDataMap[HashID].NVSItem);
		        		}

		        		CommandIRTXDataMap.erase(HashID);
		        		HashID = 0;

		        		CommandsIRLastSignalTime = Time::UptimeU();
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

		void TriggerStateChange(IRLib &Signal) {
			if (CommandIRLastSignalCRC != "")
				if (Signal.GetSignalCRC() == CommandIRLastSignalCRC)
					return;

			if (Signal.RawData.size() == 0)
				return;

			CommandIRLastSignalCRC = Signal.GetSignalCRC();

			if (Settings.eFuse.Type == Settings.Devices.Remote)
				((DataRemote_t*)Data)->SetExternalStatusByIRCommand(Signal);
		}

		// AC Codeset callbacks

		static void ACReadStarted(const char *IP) {
			RMT::TXClear();

			IRACFrequency = 38000;
			ACCode.clear();
			ACReadPart = "";
		}

		static bool ACReadBody(char* Data, int DataLen, const char *IP) {
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

		static void ACReadFinished(const char *IP) {
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

					((DataRemote_t*)Data)->SetExternalStatusForAC(Codeset, Operand);

					if (ACOperand::IsOnSeparateForCodeset(Codeset) && Operand == 0xFFF0)
						CommandIR->TXSendAddPause(500);

					Sensor_t::LocalMQTTSend(Params["operand"], "/ir/ac/sent");
				}
		}

		static void ACReadAborted(const char *IP) {
			ACCode.clear();

			RMT::TXClear();
			ESP_LOGE("CommandIR", "Failed to retrieve ac code from server");
		}
};

#endif
