/*
*    DataRemote.cpp
*    Class to handle API /Data for Remote Device
*
*/

#ifndef DATAREMOTE_H
#define DATAREMOTE_H

#include "Data.h"

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <stdio.h>
#include <string.h>

#include <esp_netif_ip_addr.h>

#include "WebServer.h"
#include "WiFi.h"
#include "Memory.h"

#include <IRLib.h>

using namespace std;

class DataRemote_t : public DataEndpoint_t {
	public:

		enum Error {
			Ok 					= 0x00,
			EmptyUUIDDevice 	= 0x01,
			DeviceAlreadyExists = 0x10,
			NoSpaceToSaveDevice = 0x11,
			DeviceNotFound		= 0x20,
			SignalsFieldEmpty	= 0x21,
			UnsupportedFunction	= 0x22,
			NoFreeSpace 		= 0x91
		};

		const string ResponseItemDidntExist 	= "{\"success\" : \"false\", \"Message\" : \"Item didn't exists\" }";
		const string ResponseItemAlreadyExist 	= "{\"success\" : \"false\", \"Message\" : \"Item already exists\" }";

		class IRDevice {
			public:
				string 		UUID 	= "";
				uint8_t 	Type 	= 0xFF;
				uint32_t	Updated = 0;
				string 		Name	= "";
				uint32_t	Extra 	= 0;

				map<string,string> Functions = map<string,string>();

				void ParseJSONItems(map<string, string> Items) {
					if (Items.size() == 0)
						return;

					if (Items.count("t")) 		{ Type 		= Converter::UintFromHexString<uint8_t>(Items["t"]); 		Items.erase("t"); }
					if (Items.count("type")) 	{ Type 		= Converter::UintFromHexString<uint8_t>(Items["type"]);		Items.erase("type"); }

					if (Items.count("n")) 		{ Name 		= Items["n"]; 		Items.erase("n"); }
					if (Items.count("name")) 	{ Name 		= Items["name"]; 	Items.erase("name"); }

					if (Items.count("e")) 		{ Extra 	= Converter::UintFromHexString<uint32_t>(Items["e"]);  		Items.erase("e"); }
					if (Items.count("extra")) 	{ Extra 	= Converter::UintFromHexString<uint32_t>(Items["extra"]); 	Items.erase("extra"); }

					if (Items.count("u")) 		{ Updated 	= Converter::ToUint32(Items["u"]); 		Items.erase("u"); }
					if (Items.count("updated")) { Updated 	= Converter::ToUint32(Items["updated"]);Items.erase("updated"); }

					if (Items.count("uuid")) 	{ UUID		= Converter::ToUpper(Items["uuid"]); 	Items.erase("uuid"); }

					for (auto& Item : Items)
						Functions[Item.first] = Item.second;

					Converter::FindAndRemove(UUID, "|");
				}

				IRDevice(string Data = "", string sUUID = "") {
					if (sUUID 	!= "") 	UUID = sUUID;
					if (Data 	== "") 	return;

					JSON JSONObject(Data);
					ParseJSONItems(JSONObject.GetItems());
				}

				IRDevice(map<string,string> Items, string sUUID = "") {
					if (sUUID != "") UUID = sUUID;
					ParseJSONItems(Items);
				}

				bool IsCorrect() {
					if (UUID == "") 	return false;
					if (Type == 0xFF) 	return false;

					return true;
				}

				string ToString(bool IsShortened = true) {
					JSON JSONObject;

					JSONObject.SetItem((IsShortened) ? "t" : "Type", Converter::ToHexString(Type, 2));
					JSONObject.SetItem((IsShortened) ? "n" : "Name", Name);
					JSONObject.SetItem((IsShortened) ? "u" : "Updated", Converter::ToString(Updated));

					if (Extra > 0)
						JSONObject.SetItem((IsShortened) ? "e" : "Extra", Converter::ToHexString(Extra,8));

					if (IsShortened) {
						for (auto& FunctionItem : Functions)
							JSONObject.SetItem(FunctionItem.first, FunctionItem.second);
					}
					else {
						vector<map<string,string>> OutputItems = vector<map<string,string>>();

						for (auto& FunctionItem : Functions)
							OutputItems.push_back({
								{ "Name", FunctionItem.first	},
								{ "Type", FunctionItem.second	}
							});

						JSONObject.SetObjectsArray("Functions", OutputItems);
					}


					return (JSONObject.ToString());
				}
		};

		void Init() override {
			NVS Memory(DataEndpoint_t::NVSArea);

			IRDevicesList = Converter::StringToVector(Memory.GetString("DevicesList"), "|");

			for (int i = 0; i< IRDevicesList.size(); i++)
				if (IRDevicesList[i] == "") IRDevicesList.erase(IRDevicesList.begin() + i);
		}

		void HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) override {
			if (Query.GetURLPartsCount() == 2)
				if (Query.CheckURLPart("deviceslist", 1)) // Forbidden to use this as UUID
				{
					Result.SetFail();
					return;
				}

			if (Query.Type == QueryType::GET) {
				if (Query.GetURLPartsCount() == 1) {
					vector<map<string,string>> OutputArray = vector<map<string,string>>();

					for (auto& IRDeviceItem : GetAvaliableDevices())
					{
						OutputArray.push_back({
							{ "UUID" 	, 	IRDeviceItem.UUID								},
							{ "Type"	, 	Converter::ToHexString(IRDeviceItem.Type,2)		},
							{ "Updated" , 	Converter::ToString(IRDeviceItem.Updated)		}
						});

					}

					JSON JSONObject;
					JSONObject.SetObjectsArray("", OutputArray);

					Result.Body = JSONObject.ToString();
					Result.ContentType = WebServer_t::Response::TYPE::JSON;
					return;
				}

				if (Query.GetURLPartsCount() == 2) {
					string UUID = Converter::ToUpper(Query.GetStringURLPartByNumber(1));

					DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

					if (DeviceItem.IsCorrect()) {
						Result.SetSuccess();
						Result.Body = DeviceItem.ToString(false);
					}
					else {
						Result.SetInvalid();
					}

					return;
				}

				if (Query.GetURLPartsCount() == 3) {
					string UUID 	= Converter::ToUpper(Query.GetStringURLPartByNumber(1));
					string Function	= Query.GetStringURLPartByNumber(2);

					JSON JSONObject;

					DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

					if (!DeviceItem.IsCorrect()) {
						Result.SetFail();
						Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(DataRemote_t::Error::DeviceNotFound)) + " }";
						return;
					}

					if (DeviceItem.Functions.count(Converter::ToLower(Function)))
						JSONObject.SetItem("Type", DeviceItem.Functions[Converter::ToLower(Function)]);
					else {
						// JSONObject.SetItem("Type", "single");
						Result.SetInvalid();
						Result.Body = ResponseItemDidntExist;
						return;
					}

					vector<map<string,string>> OutputItems = vector<map<string,string>>();

					for (IRLib Signal : LoadAllFunctionSignals(UUID, Function, DeviceItem)) {
						if (Signal.Protocol != 0xFF)
							OutputItems.push_back({
								{ "Protocol"	, Converter::ToHexString(Signal.Protocol,2) 	},
								{ "Operand"		, Converter::ToHexString(Signal.Uint32Data,8)	}
							});
						else
							OutputItems.push_back({
								{ "ProntoHEX"	, Signal.GetProntoHex(false) 	}
							});
					}

					JSONObject.SetObjectsArray("Signals", OutputItems);

					Result.Body = JSONObject.ToString();
					return;
				}
			}

			// POST запрос - сохранение и изменение данных
			if (Query.Type == QueryType::POST)
			{
				if (Query.GetURLPartsCount() == 1 || Query.GetURLPartsCount() == 2) {
					map<string,string> Params = Query.GetParams();

					string UUID = "";
					if (Params.count("uuid")) 			UUID = Converter::ToUpper(Params["uuid"]);
					if (Query.GetURLPartsCount() == 2) 	UUID = Converter::ToUpper(Query.GetStringURLPartByNumber(1));

					PUTorPOSTDeviceItem(Query, UUID, Result);

					return;
				}

				if (Query.GetURLPartsCount() == 3)
				{
					string UUID 	= Converter::ToUpper(Query.GetStringURLPartByNumber(1));
					string Function	= Query.GetStringURLPartByNumber(2);

					PUTorPOSTDeviceFunction(Query, UUID, Function, Result);
					return;
				}
			}

			// PUT запрос - обновление ранее сохраненных данных
			if (Query.Type == QueryType::PUT)
			{
				if (Query.GetURLPartsCount() == 1 || Query.GetURLPartsCount() == 2) {
					map<string,string> Params = Query.GetParams();

					string UUID = "";
					if (Params.count("uuid")) 			UUID = Converter::ToUpper(Params["uuid"]);
					if (Query.GetURLPartsCount() == 2) 	UUID = Converter::ToUpper(Query.GetStringURLPartByNumber(1));

					PUTorPOSTDeviceItem(Query, UUID, Result);

					return;
				}

				if (Query.GetURLPartsCount() == 3)
				{
					string UUID 	= Converter::ToUpper(Query.GetStringURLPartByNumber(1));
					string Function	= Query.GetStringURLPartByNumber(2);

					PUTorPOSTDeviceFunction(Query, UUID, Function, Result);
					return;
				}
			}

			if (Query.Type == QueryType::DELETE) {
				if (Query.GetURLPartsCount() == 1) {
					NVS Memory(DataEndpoint_t::NVSArea);
					IRDevicesList.clear();
					Memory.EraseNamespace();
				}

				if (Query.GetURLPartsCount() == 2) {
					NVS Memory(DataEndpoint_t::NVSArea);

					string UUID = Converter::ToUpper(Query.GetStringURLPartByNumber(1));

					DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);
					if (!DeviceItem.IsCorrect())
					{
						Result.SetFail();
						Result.Body = ResponseItemDidntExist;
						return;
					}

					RemoveFromIRDevicesList(UUID);
					Memory.EraseStartedWith(UUID);
				}

				if (Query.GetURLPartsCount() == 3) {
					string UUID 	= Converter::ToUpper(Query.GetStringURLPartByNumber(1));
					string Function	= Query.GetStringURLPartByNumber(2);

					DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

					if (!DeviceItem.IsCorrect())
					{
						Result.SetFail();
						Result.Body = ResponseItemDidntExist;
						return;
					}

					vector<string> FunctionsToDelete = Converter::StringToVector(Function, ",");

					if (!FunctionsToDelete.size()) {
						Result.SetFail();
						return;
					}

					NVS Memory(DataEndpoint_t::NVSArea);

					bool IsOK = false;
					for (auto& FunctionToDelete : FunctionsToDelete) {
						if (DeviceItem.Functions.count(FunctionToDelete) != 0)
						{
							IsOK = true;
							DeviceItem.Functions.erase(FunctionToDelete);
							Memory.EraseStartedWith(UUID + "_" + FunctionToDelete + "_");
						}
					}

					SaveDevice(DeviceItem);

					if (!IsOK && FunctionsToDelete.size() == 1)
					{
						Result.SetFail();
						Result.Body = ResponseItemDidntExist;
						return;
					}
				}
			}

			Result.SetSuccess();
		}

		void PUTorPOSTDeviceItem(Query_t &Query, string UUID, WebServer_t::Response &Result) {
			DataRemote_t::IRDevice NewDeviceItem;

			if (UUID == "")
			{
				Result.SetFail();
				return;
			}

			DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

			if (Query.Type == QueryType::POST && DeviceItem.IsCorrect())
			{
				Result.SetFail();
				Result.Body = ResponseItemAlreadyExist;
				return;
			}

			if (Query.Type == QueryType::PUT && !DeviceItem.IsCorrect())
			{
				Result.SetFail();
				Result.Body = ResponseItemDidntExist;
				return;
			}

			DeviceItem.ParseJSONItems(JSON(Query.GetBody()).GetItems());

			uint8_t ResultCode = SaveDevice(DeviceItem);
			if (ResultCode == 0)
				Result.SetSuccess();
			else
			{
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(ResultCode) + " }";
			}

			Result.SetSuccess();

			return;
		}

		void PUTorPOSTDeviceFunction(Query_t &Query, string UUID, string Function, WebServer_t::Response &Result) {
			JSON JSONObject(Query.GetBody());

			DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

			if (!DeviceItem.IsCorrect()) {
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(DataRemote_t::Error::DeviceNotFound)) + " }";
				return;
			}

			if (!JSONObject.IsItemExists("signals"))
			{
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(DataRemote_t::Error::SignalsFieldEmpty)) + " }";
				return;
			}

			if (Query.Type == QueryType::POST && DeviceItem.Functions.count(Function) > 0)
			{
				Result.SetFail();
				Result.Body = ResponseItemAlreadyExist;
				return;
			}

			if (Query.Type == QueryType::PUT && DeviceItem.Functions.count(Function) == 0)
			{
				Result.SetFail();
				Result.Body = ResponseItemDidntExist;
				return;
			}

			if (!CheckIsValidKeyForType(DeviceItem.Type, Function)) {
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Message\" : \"Invalid function name\" }";
				return;
			}

			if (Query.Type == QueryType::PUT) {
				NVS Memory(DataEndpoint_t::NVSArea);
				Memory.EraseStartedWith(UUID + "_" + Function + "_");
			}

			JSON SignalsJSON = JSONObject.Detach("signals");
			SignalsJSON.SetDestructable(false);

			if (SignalsJSON.GetType() == JSON::RootType::Array) {
				cJSON *JSONRoot = SignalsJSON.GetRoot();

				cJSON *Child = JSONRoot->child;

				int Index = 0;

				string SignalType = "single";
				if (JSONObject.IsItemExists("type")) SignalType = JSONObject.GetItem("type");

				DeviceItem.Functions[Function] =  SignalType;

				if (JSONObject.IsItemExists("updated"))
					DeviceItem.Updated = Converter::ToUint32(JSONObject.GetItem("updated"));

				uint8_t SaveDeviceResult = SaveDevice(DeviceItem);

				if (SaveDeviceResult)
				{
					Result.SetFail();
					Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(SaveDeviceResult) + " }";
					return;
				}

				while(Child) {
					JSON ChildObject(Child);
					ChildObject.SetDestructable(false);

					uint8_t SaveFunctionResult = SaveFunction(UUID, Function, SerializeIRSignal(ChildObject), Index++, DeviceItem.Type);
					if (SaveFunctionResult)
					{
						Result.SetFail();
						Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(SaveFunctionResult) + " }";
						return;
					}

					Child = Child->next;
				}
			}
			else
			{
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(DataRemote_t::Error::SignalsFieldEmpty)) + " }";
				return;
			}

			Result.SetSuccess();
			return;
		}

		map<string,string> LoadDeviceFunctions(string UUID) {
			NVS Memory(DataEndpoint_t::NVSArea);

			IRDevice Result(Memory.GetString(UUID), UUID);

			if (Result.IsCorrect())
				return Result.Functions;

			return map<string,string>();
		}


		pair<bool,IRLib> LoadFunctionByIndex(string UUID, string Function, uint8_t Index = 0x0, IRDevice DeviceItem = IRDevice()) {
			UUID 	= Converter::ToUpper(UUID);
			Function= Converter::ToLower(Function);

			NVS Memory(DataEndpoint_t::NVSArea);

			IRLib Result;

			if (!DeviceItem.IsCorrect())
				DeviceItem = LoadDevice(UUID);

			if (DeviceItem.Functions.count(Function) > 0) {
				string KeyString = UUID + "_" + Function;

				if (DeviceItem.Functions[Function] == "single")
					return DeserializeIRSignal(Memory.GetString(KeyString));
				else {
					if (Index > 0)
						KeyString += "_" + Converter::ToString(Index);

					return DeserializeIRSignal(Memory.GetString(KeyString));
				}
			}

			return make_pair(false, Result);
		}

		vector<IRLib> LoadAllFunctionSignals(string UUID, string Function, IRDevice DeviceItem = IRDevice()) {
			UUID 		= Converter::ToUpper(UUID);
			Function	= Converter::ToLower(Function);

			if (!DeviceItem.IsCorrect())
				DeviceItem = LoadDevice(UUID);

			vector<IRLib> Result = vector<IRLib>();

			if (DeviceItem.Functions.count(Function) > 0) {
				if (DeviceItem.Functions[Function] == "single")
					Result.push_back(LoadFunctionByIndex(UUID, Function, 0, DeviceItem).second);
				else {
					string KeyString = UUID + "_" + Function;

					for (int i = 0; i < Settings.Data.MaxIRItemSignals; i++) {
						pair<bool, IRLib> CurrentSignal = LoadFunctionByIndex(UUID, Function, i, DeviceItem);

						if (CurrentSignal.first) Result.push_back(CurrentSignal.second);
					}
				}
			}

			return Result;
		}

		string GetFunctionType(string UUID, string Function, IRDevice DeviceItem = IRDevice()) {
			UUID 		= Converter::ToUpper(UUID);
			Function	= Converter::ToLower(Function);

			if (!DeviceItem.IsCorrect())
				DeviceItem = LoadDevice(UUID);

			if (DeviceItem.Functions.count(Function) > 0)
				return DeviceItem.Functions[Function];
			else
				return "single";
		}

		string GetFunctionNameByID(uint8_t FunctionID) {
			for (auto& Function : GlobalFunctions)
				if (Function.second == FunctionID)
					return Function.first;

			return "";
		}

		uint8_t GetFunctionIDByName(string FunctionName) {
			if (GlobalFunctions.count(FunctionName) == 0)
				return 0;

			return GlobalFunctions[FunctionName];
		}

		vector<IRDevice> GetAvaliableDevices() {
			NVS Memory(DataEndpoint_t::NVSArea);

			vector<IRDevice> Result = vector<IRDevice>();

			for (auto& IRDeviceUUID: IRDevicesList) {
				DataRemote_t::IRDevice DeviceItem(Memory.GetString(IRDeviceUUID), IRDeviceUUID);

				if (DeviceItem.IsCorrect())
					Result.push_back(DeviceItem);
			}

			return Result;
		}

		private:
			vector<string>				IRDevicesList		= vector<string>();
			map<string,vector<string>> 	AvaliableFunctions 	= map<string,vector<string>>();

			uint8_t	SaveIRDevicesList() {
				NVS Memory(DataEndpoint_t::NVSArea);
				return (Memory.SetString("DevicesList", Converter::VectorToString(IRDevicesList, "|"))) ? 0 : static_cast<uint8_t>(NoSpaceToSaveDevice);
			}

			uint8_t	AddToDevicesList(string UUID) {
				if (!(std::count(IRDevicesList.begin(), IRDevicesList.end(), UUID)))
				{
					IRDevicesList.push_back(UUID);
					uint8_t SetResult = SaveIRDevicesList();

					return (SetResult != 0) ? SetResult + 0x10 : 0;
				}

				return 0;//static_cast<uint8_t>(Error::DeviceAlreadyExists);
			}

			uint8_t RemoveFromIRDevicesList(string UUID) {
				std::vector<string>::iterator itr = std::find(IRDevicesList.begin(), IRDevicesList.end(), UUID);
				if (itr != IRDevicesList.end()) IRDevicesList.erase(itr);

				return SaveIRDevicesList();
			}

			IRDevice LoadDevice(string UUID) {
				NVS Memory(DataEndpoint_t::NVSArea);

				UUID = Converter::ToUpper(UUID);

				IRDevice Result(Memory.GetString(UUID), UUID);
				return Result;
			}

			uint8_t	SaveDevice(IRDevice DeviceItem) {
				NVS Memory(DataEndpoint_t::NVSArea);

				bool SetResult = Memory.SetString(DeviceItem.UUID, DeviceItem.ToString());

				if (SetResult != true) return 1;
				return AddToDevicesList(DeviceItem.UUID);
			}

			uint8_t SaveFunction(string UUID, string Function, string Item, uint8_t Index, uint8_t DeviceType) {
				NVS Memory(DataEndpoint_t::NVSArea);

				if (!CheckIsValidKeyForType(DeviceType, Function))
					return static_cast<uint8_t>(DataRemote_t::Error::UnsupportedFunction);

				string ValueName = UUID + "_" + Function;

				if (Index > 0)
					ValueName += "_" + Converter::ToString(Index);

				Memory.SetString(ValueName, Item);

				return 0;
			}

			string SerializeIRSignal (JSON &JSONObject) {
				IRLib Signal;

				if (JSONObject.IsItemExists("raw")) { // сигнал передан в виде Raw
					map<string,string> RawData = JSONObject.GetItemsForKey("raw");

					if (RawData.count("signal") 	> 0)	Signal.LoadFromRawString(RawData["signal"]);
					if (RawData.count("frequency")	> 0)	Signal.Frequency = Converter::ToUint16(RawData["frequency"]);
				}
				else if (JSONObject.IsItemExists("prontohex")) { // сигнал передан в виде prontohex
					string ProntoHEX = JSONObject.GetItem("prontohex");
					Signal = IRLib(ProntoHEX);
				}
				else if (JSONObject.IsItemExists("protocol")){ // сигнал передан  в виде протокола
					if (JSONObject.IsItemExists("protocol"))
						Signal.Protocol = Converter::UintFromHexString<uint8_t>(JSONObject.GetItem("protocol"));

					if (JSONObject.IsItemExists("operand"))
						Signal.Uint32Data = Converter::UintFromHexString<uint32_t>(JSONObject.GetItem("operand"));
				}

				if (Signal.Protocol != 0xFF)
					return Converter::ToHexString(Signal.Protocol,2) + Converter::ToHexString(Signal.Uint32Data, 8);
				else if (Signal.Protocol == 0xFF && Signal.RawData.size() > 0)
					return Signal.GetProntoHex(false);
				else
					return "";
			}

			pair<bool, IRLib> DeserializeIRSignal(string Item) {
				if (Item.size() < 10)
					return make_pair(false, IRLib());

				if (Item.size() == 10)
				{
					IRLib Result;

					Result.Protocol 	= Converter::ToUint8(Item.substr(0, 2));
					Result.Uint32Data	= Converter::UintFromHexString<uint32_t>(Item.substr(2));

					return make_pair(true, Result);
				}

				return make_pair(true, IRLib(Item));
			}

			map<string, uint8_t> GlobalFunctions =
			{
				{ "power"	, 0x01 }, { "poweron"	, 0x02 }, { "poweroff"	, 0x03 	},
				{ "mode"	, 0x04 },
				{ "mute"	, 0x05 }, { "volup"		, 0x06 }, { "voldown"	, 0x07 	},
				{ "chup"	, 0x08 }, { "chdown"	, 0x09 },
				{ "chup"	, 0x08 }, { "chdown"	, 0x09 },
				{ "swing"	, 0x0A }, { "speed"		, 0x0B }
			};

			// Device types:
			// 0x00 Не указано / Прочее
			// 0x01 TV
			// 0x02 Медиа-центр
			// 0x03 Light
			// 0x04 Увлажнитель
			// 0x05 Очиститель воздуха
			// 0x06 Робот пылесос
			// 0x07 Вентилятор
			// 0xEF Кондиционеры


			bool CheckIsValidKeyForType(uint8_t Type, string Key) {
				vector<string> AvaliableKeys = vector<string>();

				if (Type == 0 || Type == 0x3 || Type == 0x6) {
					AvaliableKeys.push_back("power");
					AvaliableKeys.push_back("poweron");
					AvaliableKeys.push_back("poweroff");
				}

				if (Type == 0x1) {
					AvaliableKeys.push_back("power");		// power
					AvaliableKeys.push_back("poweron");
					AvaliableKeys.push_back("poweroff");

					AvaliableKeys.push_back("mode");		// mode

					AvaliableKeys.push_back("mute");		// mute
					AvaliableKeys.push_back("volup");		// volume up
					AvaliableKeys.push_back("voldown");		// volume down

					AvaliableKeys.push_back("chup");		// channel up
					AvaliableKeys.push_back("chdown");		// channel down
				}

				if (Type == 0x2) {
					AvaliableKeys.push_back("power");
					AvaliableKeys.push_back("poweron");
					AvaliableKeys.push_back("poweroff");

					AvaliableKeys.push_back("mute");
					AvaliableKeys.push_back("volup");
					AvaliableKeys.push_back("voldown");
				}

				if (Type == 0x7) {
					AvaliableKeys.push_back("power");
					AvaliableKeys.push_back("poweron");
					AvaliableKeys.push_back("poweroff");

					AvaliableKeys.push_back("swing");
					AvaliableKeys.push_back("mode");
				}

				if (Type == 0x5) {
					AvaliableKeys.push_back("power");
					AvaliableKeys.push_back("poweron");
					AvaliableKeys.push_back("poweroff");

					AvaliableKeys.push_back("swing");
					AvaliableKeys.push_back("mode");
					AvaliableKeys.push_back("speed");
				}

				if (Type == 0x4) {
					AvaliableKeys.push_back("power");
					AvaliableKeys.push_back("poweron");
					AvaliableKeys.push_back("poweroff");

					AvaliableKeys.push_back("swing");
					AvaliableKeys.push_back("mode");
				}

				if(std::find(AvaliableKeys.begin(), AvaliableKeys.end(), Converter::ToLower(Key)) != AvaliableKeys.end())
				    return true;
				else
					return false;
			}

			//static string 	SerializeIRSignal(IRLib &);

};

#endif

