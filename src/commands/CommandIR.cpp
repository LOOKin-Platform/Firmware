/*
*    CommandIR.cpp
*    CommandIR_t implementation
*
*/
#ifndef COMMANDS_IR
#define COMMANDS_IR

#include <RMT.h>
#include "Sensors.h"

#include "Data.h"

extern DataEndpoint_t *Data;

static rmt_channel_t TXChannel = RMT_CHANNEL_0;

static string 				IRACReadBuffer 			= "";
static uint16_t				IRACFrequency			= 38000;

static string 				ProntoHexBlockedBuffer 	= "";
static esp_timer_handle_t 	ProntoHexBlockedTimer 	= NULL;

//static FreeRTOS::Semaphore 	IsIRSignalReadyToSend 	= FreeRTOS::Semaphore("IsIRSignalReadyToSend");


class CommandIR_t : public Command_t {
	public:
		struct LastSignal_t {
			uint8_t 	Protocol 	= 0;
			uint32_t 	Data		= 0;
			uint16_t 	Misc		= 0;
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

				RMT::TXSetItems(IRSignal.GetRawDataForSending());
				TXSend(IRSignal.GetProtocolFrequency());
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

				RMT::TXSetItems(RepeatedSignal);
				TXSend(IRSignal.GetProtocolFrequency());

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

						RMT::TXSetItems(IRSignal.GetRawDataForSending());
						TXSend(IRSignal.Frequency);
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

				ESP_LOGE("QUERY", "%s", (Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery()).c_str());

				HTTPClient::Query(	Settings.ServerUrls.GetACCode + "?" + ACData.GetQuery(),
									QueryType::POST, false, true,
									&ACReadStarted,
									&ACReadBody,
									&ACReadFinished,
									&ACReadAborted);

				return true;
			}

			if (EventCode == 0xF0) { // Send in ProntoHex
				if (::strcmp(StringOperand, "0") == 0 || InOperation)
					return false;

				IRLib *IRSignal = new IRLib(StringOperand);

				LastSignal.Protocol	= IRSignal->Protocol;
				LastSignal.Data 	= IRSignal->Uint32Data;
				LastSignal.Misc		= IRSignal->MiscData;

				RMT::TXSetItems(IRSignal->GetRawDataForSending());
				TXSend(IRSignal->Frequency);
				TriggerStateChange(*IRSignal);

				delete IRSignal;
				return true;
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

				vector<IRLib> SignalsToSend = vector<IRLib>();

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

				if (SignalID != 0xFF) {
					pair<bool,IRLib> SignalToAdd = ((DataRemote_t*)Data)->LoadFunctionByIndex(UUID, Function, SignalID);

					ESP_LOGE("SignalToAdd", "%s", (SignalToAdd.first == true) ? "true" : "false");

					if (SignalToAdd.first)
						SignalsToSend.push_back(SignalToAdd.second);
				}
				else
					SignalsToSend = ((DataRemote_t*)Data)->LoadAllFunctionSignals(UUID, Function);

			    for(auto it = SignalsToSend.begin(); it != SignalsToSend.end();) {
					LastSignal.Protocol = it->Protocol;
					LastSignal.Data		= it->Uint32Data;
					LastSignal.Misc		= it->MiscData;

					if (Operand == 0x0 && Misc == 0x0)
						return false;

					RMT::TXSetItems(it->GetRawDataForSending());
					TXSend(it->GetProtocolFrequency());

			    	it = SignalsToSend.erase(it);
			    }

				((DataRemote_t*)Data)->StatusUpdateForDevice(UUID, FunctionID, SignalID);

				return true;
			}

			if (EventCode == 0xFF) { // Send RAW Format
				if (::strcmp(StringOperand, "0") == 0 || InOperation)
					return false;

				uint16_t Frequency = 38000;

				char *FrequencyDelimeterPointer = strstr(StringOperand, ";");
				if (FrequencyDelimeterPointer != NULL) {
					size_t FrequencyDelimeterPos =  FrequencyDelimeterPointer - StringOperand;
					Frequency = Converter::ToUint16(string(StringOperand, FrequencyDelimeterPos));
					StringOperand += FrequencyDelimeterPos + 1;
				}

				if (::strstr(StringOperand," ") == NULL && ::strstr(StringOperand,"%20") == NULL)
					return false;

				IRLib *IRSignal = new IRLib();

				string 		SignalItem 	= "";
				char		SignalChar[1];

				for (int i = 0; i < strlen(StringOperand); i++) {
					memcpy(SignalChar, StringOperand + i, 1);
					string SignalCharStr(SignalChar,1);

					if ((SignalCharStr == string(" ")) || (SignalCharStr == string("%")))
					{
						if (SignalCharStr == string("%")) i+=2;

						IRSignal->RawData.push_back(Converter::ToInt32(SignalItem));
						SignalItem = "";
					}
					else
						SignalItem += SignalCharStr;
				}
				IRSignal->RawData.push_back(Converter::ToInt32(SignalItem));
				IRSignal->ExternFillPostOperations();

				LastSignal.Protocol = IRSignal->Protocol;
				LastSignal.Data 	= IRSignal->Uint32Data;
				LastSignal.Misc		= IRSignal->MiscData;

				if (LastSignal.Protocol != 0xFF)
					RMT::TXSetItems(IRSignal->GetRawDataForSending());
				else {
					RMT::TXClear();

					while (IRSignal->RawData.size())
						RMT::TXAddItem(IRSignal->RawPopItem());
				}

				TXSend(Frequency);
				TriggerStateChange(*IRSignal);

				delete IRSignal;
				return true;
			}

			return false;
		}

		static void ProntoHexBlockedCallback(void *Param) {
			ProntoHexBlockedBuffer = "";
		}

		void TXSend(uint16_t Frequency) {
			//IsIRSignalReadyToSend.Wait("IsIRSignalReadyToSend");
			//IsIRSignalReadyToSend.Take("IsIRSignalReadyToSend");


			if (RMT::TXItemsCount() == 0)
				return;

			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0)
				RMT::UnsetRXChannel(RMT_CHANNEL_0);

			InOperation = true;

			vector<gpio_num_t> GPIO = Settings.GPIOData.GetCurrent().IR.SenderGPIOs;

			if (Settings.GPIOData.GetCurrent().IR.SenderGPIOExt != GPIO_NUM_0)
				GPIO.push_back(Settings.GPIOData.GetCurrent().IR.SenderGPIOExt);

			if (Settings.eFuse.Type == Settings.Devices.Remote && Settings.eFuse.Model > 1) {
				// deinit all pins for gpio output
				for (auto& PinItem : GPIO)
					gpio_matrix_out(PinItem, 0x100, 0, 0);

				((DataRemote_t *)Data)->ClearChannels(GPIO);
			}

			RMT::TXSend(GPIO, TXChannel, Frequency);
			InOperation = false;

			Log::Add(Log::Events::Commands::IRExecuted);

			if (Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38 != GPIO_NUM_0) {
				RMT::SetRXChannel(Settings.GPIOData.GetCurrent().IR.ReceiverGPIO38, RMT_CHANNEL_0, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
				RMT::ReceiveStart(RMT_CHANNEL_0);
			}

			//IsIRSignalReadyToSend.Give();
		}

		void TriggerStateChange(IRLib &Signal) {
			if (Settings.eFuse.Type == Settings.Devices.Remote)
				((DataRemote_t*)Data)->SetExternalStatusByIRCommand(Signal);
		}

		// AC Codeset callbacks

		static void ACReadStarted(const char *IP) {
			RMT::TXClear();

			IRACFrequency 		= 38000;
			IRACReadBuffer 		= "";
		}

		static bool ACReadBody(char Data[], int DataLen, const char *IP) {
			IRACReadBuffer += string(Data, DataLen);

			size_t FrequencyDelimeterPos = IRACReadBuffer.find(";");
			if (FrequencyDelimeterPos != std::string::npos) {
				IRACFrequency 	= Converter::ToUint16(IRACReadBuffer.substr(0, FrequencyDelimeterPos));
				IRACReadBuffer 	= IRACReadBuffer.substr(FrequencyDelimeterPos+1);
			}

			if (IRACReadBuffer.find(" ") == string::npos)
				return true;

			while (IRACReadBuffer.size() > 0)
			{
				size_t Pos = IRACReadBuffer.find(" ");

				if (Pos != string::npos) {
					RMT::TXAddItem(Converter::ToInt32(IRACReadBuffer.substr(0,Pos)));
					//ESP_LOGE("TXITEM", "%d", Converter::ToInt32(IRACReadBuffer.substr(0,Pos)));
					IRACReadBuffer.erase(0, Pos + 1);
				}
				else
					break;
			}

			return true;
		};

		static void ACReadFinished(const char *IP) {
			if (IRACReadBuffer.size() > 0) {
				RMT::TXAddItem(Converter::ToInt32(IRACReadBuffer));
				//ESP_LOGE("TXITEM", "%d", Converter::ToInt32(IRACReadBuffer));
				IRACReadBuffer = "";
			}

			CommandIR_t* CommandIR = (CommandIR_t*)Command_t::GetCommandByName("IR");
			if (CommandIR == nullptr)
				ESP_LOGE("CommandIR","Can't get IR command");
			else
				CommandIR->TXSend(IRACFrequency);

			Query_t Query(IP, WebServer_t::QueryTransportType::WebServer);
			map<string,string> Params = Query.GetParams();

			if (Params.count("operand") > 0 && Settings.eFuse.Type == Settings.Devices.Remote)
				if (Params["operand"].size() == 8)
					((DataRemote_t*)Data)->SetExternalStatusForAC(
							Converter::UintFromHexString<uint16_t>(Params["operand"].substr(0, 4)),
							Converter::UintFromHexString<uint16_t>(Params["operand"].substr(4, 4)));
		}

		static void ACReadAborted(const char *IP) {
			IRACReadBuffer = "";

			RMT::TXClear();
			ESP_LOGE("CommandIR", "Failed to retrieve ac code from server");
		}
};

#endif
