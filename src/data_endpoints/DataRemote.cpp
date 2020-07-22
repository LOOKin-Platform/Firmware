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

		class IRDevice {
			public:
				void ParseJSONItems(map<string, string> Items) {
					if (Items.size() == 0)
						return;

					if (Items.count("t")) 		{ Type 		= Converter::ToUint8(Items["t"]); 		Items.erase("t"); }
					if (Items.count("type")) 	{ Type 		= Converter::ToUint8(Items["type"]);	Items.erase("type"); }

					if (Items.count("n")) 		{ Name 		= Items["n"]; 		Items.erase("n"); }
					if (Items.count("name")) 	{ Name 		= Items["name"]; 	Items.erase("name"); }

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

					JSONObject.SetItem((IsShortened) ? "t" : "Type", Converter::ToString(Type));
					JSONObject.SetItem((IsShortened) ? "n" : "Name", Name);
					JSONObject.SetItem((IsShortened) ? "u" : "Updated", Converter::ToString(Updated));

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

				string 		UUID 	= "";
				uint8_t 	Type 	= 0xFF;
				uint32_t	Updated = 0;
				string 		Name	= "";

				map<string,string> Functions = map<string,string>();
		};

		void Init() override {
			NVS Memory(DataEndpoint_t::NVSArea);

			IRDevicesList = Converter::StringToVector(Memory.GetString("DevicesList"), "|");

			for (int i = 0; i< IRDevicesList.size(); i++)
				if (IRDevicesList[i] == "") IRDevicesList.erase(IRDevicesList.begin() + i);
		}

		void HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody, httpd_req_t *Request,  WebServer_t::QueryTransportType TransportType, int MsgID) override {
			if (URLParts.size() == 1)
				if (Converter::ToLower(URLParts[0]) == "deviceslist") // Forbidden to use this as UUID
				{
					Result.SetFail();
					return;
				}

			if (Type == QueryType::GET) {
				if (URLParts.size() == 0) {

					NVS Memory(DataEndpoint_t::NVSArea);
					vector<map<string,string>> OutputArray = vector<map<string,string>>();

					for (auto& IRDeviceUUID: IRDevicesList) {
						DataRemote_t::IRDevice DeviceItem(Memory.GetString(IRDeviceUUID), IRDeviceUUID);

						OutputArray.push_back({
							{ "UUID" 	, 	DeviceItem.UUID							},
							{ "Type"	, 	Converter::ToString(DeviceItem.Type)	},
							{ "Updated" , 	Converter::ToString(DeviceItem.Updated)	}
						});
					}

					JSON JSONObject;
					JSONObject.SetObjectsArray("", OutputArray);

					Result.Body = JSONObject.ToString();
					Result.ContentType = WebServer_t::Response::TYPE::JSON;
					return;
				}

				if (URLParts.size() == 1)
				{
					string UUID = Converter::ToUpper(URLParts[0]);

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

				if (URLParts.size() == 2)
				{
					string UUID 	= Converter::ToUpper(URLParts[0]);
					string Function	= URLParts[1];

					JSON JSONObject;

					DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

					if (!DeviceItem.IsCorrect()) {
						Result.SetFail();
						Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(DataRemote_t::Error::DeviceNotFound)) + " }";
						return;
					}

					if (DeviceItem.Functions.count(Converter::ToLower(Function)))
						JSONObject.SetItem("Type", DeviceItem.Functions[Converter::ToLower(Function)]);
					else
						JSONObject.SetItem("Type", "single");

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
			if (Type == QueryType::POST)
			{
				if (URLParts.size() < 2) {
					DataRemote_t::IRDevice NewDeviceItem;

					string UUID = "";
					if (Params.count("uuid")) UUID = Converter::ToUpper(Params["uuid"]);
					if (URLParts.size() == 1) UUID = Converter::ToUpper(URLParts[0]);

					if (UUID == "")
					{
						Result.SetFail();
						return;
					}

					DataRemote_t::IRDevice DeviceItem = LoadDevice(UUID);

					if (DeviceItem.IsCorrect())
						DeviceItem.ParseJSONItems(Params);
					else
						DeviceItem = DataRemote_t::IRDevice(Params, UUID);

					if (!DeviceItem.IsCorrect())
					{
						Result.SetFail();
						return;
					}

					uint8_t ResultCode = SaveDevice(DeviceItem);
					if (ResultCode == 0)
						Result.SetSuccess();
					else
					{
						Result.SetFail();
						Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(ResultCode) + " }";
					}

					return;
				}

				if (URLParts.size() == 2)
				{
					string UUID 	= Converter::ToUpper(URLParts[0]);
					string Function	= URLParts[1];

					JSON JSONObject(RequestBody);

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
				}
			}

			if (Type == QueryType::DELETE) {
				if (URLParts.size() == 0) {
					NVS Memory(DataEndpoint_t::NVSArea);
					IRDevicesList.clear();
					Memory.EraseNamespace();
				}

				if (URLParts.size() == 1) {
					NVS Memory(DataEndpoint_t::NVSArea);

					string UUID = Converter::ToUpper(URLParts[0]);

					RemoveFromIRDevicesList(UUID);
					Memory.EraseStartedWith(UUID);
				}
			}

			Result.SetSuccess();
		}

		private:
			vector<string>	IRDevicesList	= vector<string>();
			map<string,vector<string>> AvaliableFunctions = map<string,vector<string>>();

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
					AvaliableKeys.push_back("volumeup");
					AvaliableKeys.push_back("volumedown");
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

