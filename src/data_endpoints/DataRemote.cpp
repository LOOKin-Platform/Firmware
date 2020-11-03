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

#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
#include "Custom.h"
#endif

using namespace std;

class DataDeviceItem_t {
	public:
		uint8_t DeviceTypeID = 0;
		virtual vector<uint8_t> GetAvaliableFunctions() { return vector<uint8_t>(); }
		virtual pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value,  string FunctionType = "") {
			return make_pair(Status, Value);
		}

		virtual ~DataDeviceItem_t() {};

		static uint8_t GetStatusByte(uint16_t Status, uint8_t ByteID) {
			if (ByteID > 3) ByteID = 3;

			Status = Status << (ByteID * 4);
			Status = Status >> 12;

			return (uint8_t)Status;
		}

		static uint16_t SetStatusByte(uint16_t Status, uint8_t ByteID, uint8_t Value) {
			switch (ByteID) {
				case 0: Status = (Status & 0x0FFF); break;
				case 1: Status = (Status & 0xF0FF); break;
				case 2: Status = (Status & 0xFF0F); break;
				case 3: Status = (Status & 0xFFF0); break;
				default: break;
			}

			Status = Status + ((uint16_t)Value << ((3 - ByteID) * 4));
			return Status;
		}

		pair<bool,uint8_t> CheckPowerUpdated(uint8_t FunctionID, uint8_t Value, uint8_t CurrentValue = 0, string FunctionType = "") { // current 0 if off and 1 if on
			if (FunctionID != 0x1 && FunctionID != 0x2 && FunctionID != 0x3)
				return make_pair(false, 0);

			if (FunctionType == "") FunctionType = "single";
			if (CurrentValue > 1) CurrentValue = 1;

			if (FunctionID == 0x1) {
				if (FunctionType == "single")
					return make_pair(true, (1-CurrentValue));
				else if (FunctionType == "toggle")
					return make_pair(true, (Value == 0) ? 1 : 0);
			}
			else if (FunctionID == 0x2)
				return make_pair(true, 1);
			else if (FunctionID == 0x3)
				return make_pair(true, 0);

			return make_pair(false, 0);
		}
};

class DataDeviceTV_t  : public DataDeviceItem_t {
	public:
		DataDeviceTV_t() { DeviceTypeID = 0x01; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0C, 0x0D}; }

		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override {

			// On/off toggle functions
			if (FunctionID == 0x1 || FunctionID == 0x2 || FunctionID == 0x3) {
				uint8_t CurrentPower = (uint8_t)(Status >> 12);

				pair<bool, uint8_t> Result = CheckPowerUpdated(FunctionID, Value, CurrentPower, FunctionType);

				if (Result.first)
					return make_pair(SetStatusByte(Status, 0, Result.second), Result.second);
			}
			else if (FunctionID == 0x04) { // mode
				Status = SetStatusByte(Status, 1, Value);
			}
			else if (FunctionID == 0x05) { // mute
				Status = SetStatusByte(Status, 2, (GetStatusByte(Status,2) > 0) ? 0 : 1);
				Value = (GetStatusByte(Status,2) == 0) ? 1 : 0;
			}
			else if (FunctionID == 0x06) { // volume up
				Status = SetStatusByte(Status, 2, 255);
			}
			else if (FunctionID == 0x07) { // volume down
				Status = SetStatusByte(Status, 2, 1);
			}

			return make_pair(Status, Value);
		}


		virtual ~DataDeviceTV_t() {};
};

class DataDeviceMedia_t  : public DataDeviceItem_t {
	public:
		DataDeviceMedia_t() { DeviceTypeID = 0x02; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03, 0x05, 0x06, 0x07}; }
		virtual ~DataDeviceMedia_t() {};
};

class DataDeviceLight_t  : public DataDeviceItem_t {
	public:
		DataDeviceLight_t() { DeviceTypeID = 0x03; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03}; }
		virtual ~DataDeviceLight_t() {};
};

class DataDeviceHumidifier_t  : public DataDeviceItem_t {
	public:
		DataDeviceHumidifier_t() { DeviceTypeID = 0x04; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03, 0x04 }; }
		virtual ~DataDeviceHumidifier_t() {};
};

class DataDeviceAirPurifier_t  : public DataDeviceItem_t {
	public:
		DataDeviceAirPurifier_t() { DeviceTypeID = 0x05; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03, 0x04, 0x0B}; }
		virtual ~DataDeviceAirPurifier_t() {};
};

class DataDeviceRoboCleaner_t  : public DataDeviceItem_t {
	public:
		DataDeviceRoboCleaner_t() { DeviceTypeID = 0x06; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03 }; }
		virtual ~DataDeviceRoboCleaner_t() {};
};

class DataDeviceFan_t  : public DataDeviceItem_t {
	public:
		DataDeviceFan_t() { DeviceTypeID = 0x07; }
		vector<uint8_t> GetAvaliableFunctions() override  { return { 0x01, 0x02, 0x03, 0x0A, 0x0B  }; }

		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override {

			// On/off toggle functions
			if (FunctionID == 0x1 || FunctionID == 0x2 || FunctionID == 0x3) {
				uint8_t CurrentPower = GetStatusByte(Status, 0);

				pair<bool, uint8_t> Result = CheckPowerUpdated(FunctionID, Value, CurrentPower, FunctionType);

				if (Result.first)
					return make_pair(SetStatusByte(Status, 0, Result.second), Result.second);
			}

			return make_pair(Status, Value);
		}

		virtual ~DataDeviceFan_t() {};
};

class DataDeviceAC_t : public DataDeviceItem_t {
	public:
		DataDeviceAC_t() { DeviceTypeID = 0xEF; }

		pair<uint16_t, uint8_t> UpdateStatusForFunction (uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override
		{
			ACOperand Operand((uint32_t)Status);

			switch (FunctionID) {
				case 0xE0: Operand.Mode 		= Value; break;
				case 0xE1: Operand.Temperature 	= Value; break;
				case 0xE2: Operand.FanMode 		= Value; break;
				case 0xE3: Operand.SwingMode	= Value; break;
			}

			return make_pair((uint16_t)((Operand.ToUint32() << 16) >> 16), Value);
		}

		virtual ~DataDeviceAC_t() {};
};

class DataDevice_t {
	public:
		DataDevice_t() {
			DevicesClasses = {
					new DataDeviceTV_t(), new DataDeviceMedia_t(), new DataDeviceLight_t(),
					new DataDeviceHumidifier_t(), new DataDeviceAirPurifier_t(),
					new DataDeviceRoboCleaner_t(), new DataDeviceFan_t(), new DataDeviceAC_t() };
		}

		~DataDevice_t() {
			for (auto& DeviceClass : DevicesClasses)
				delete DeviceClass;
		}

		uint8_t FunctionIDByName(string FunctionName) {
			return (GlobalFunctions.count(FunctionName) == 0) ? 0 : GlobalFunctions[FunctionName];
		}

		string FunctionNameByID(uint8_t FunctionID) {
			for (auto& Function : GlobalFunctions)
				if (Function.second == FunctionID)
					return Function.first;
			return "";
		}

		DataDeviceItem_t* GetDeviceForType(uint8_t Type) {
			for (auto& DeviceClass : DevicesClasses)
				if (DeviceClass->DeviceTypeID == Type)
					return DeviceClass;

			return nullptr;
		}

		bool IsValidKeyForType(uint8_t Type, string Key) {
			DataDeviceItem_t *DeviceItem = GetDeviceForType(Type);

			if (DeviceItem == nullptr) return false;

			Key = Converter::ToLower(Key);

			for (auto& FunctionID : DeviceItem->GetAvaliableFunctions())
				if (Key == FunctionNameByID(FunctionID))
					return true;

			return false;
		}

		//void SetStatusForType

	private:
		vector<DataDeviceItem_t *> 	DevicesClasses = vector<DataDeviceItem_t *>();

		map<string, uint8_t> GlobalFunctions =
		{
			{ "power"	, 0x01 }, { "poweron"	, 0x02 }, { "poweroff"	, 0x03 	},
			{ "mode"	, 0x04 },
			{ "mute"	, 0x05 }, { "volup"		, 0x06 }, { "voldown"	, 0x07 	},
			{ "chup"	, 0x08 }, { "chdown"	, 0x09 },
			{ "swing"	, 0x0A }, { "speed"		, 0x0B },
			{ "cursor"	, 0x0C },
			{ "menu"	, 0x0D },
			// virtual AC functions
			{ "acmode"	, 0xE0 }, { "actemp", 0xE1 }, { "acfan", 0xE2 }, { "acswing", 0xE3 }
		};


};

class DataRemote_t : public DataEndpoint_t {
	public:

		DataDevice_t 	DevicesHelper;

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

		struct IRDeviceCacheItem_t {
			string 			DeviceID 	{ ""  };
			uint8_t			DeviceType 	{ 0x0 };
			uint16_t		Status 		{ 0x0 };
			uint16_t		Extra		{ 0x0 };
			map<uint8_t, pair<string,vector<pair<uint8_t,uint32_t>>>>
							Functions = map<uint8_t, pair<string,vector<pair<uint8_t,uint32_t>>>>();

			bool IsEmpty() { return (DeviceID == "" && DeviceType == 0x0); }
		};

		vector<IRDeviceCacheItem_t> IRDevicesCache = vector<IRDeviceCacheItem_t>();

		class IRDevice {
			public:
				string 		UUID 	= "";
				uint8_t 	Type 	= 0xFF;
				uint32_t	Updated = 0;
				string 		Name	= "";
				uint16_t	Extra 	= 0;
				uint16_t	Status	= 0;

				map<string,string> Functions = map<string,string>();

				void ParseJSONItems(map<string, string> Items) {
					if (Items.size() == 0)
						return;

					if (Items.count("t")) 		{ Type 		= Converter::UintFromHexString<uint8_t>(Items["t"]); 		Items.erase("t"); }
					if (Items.count("type")) 	{ Type 		= Converter::UintFromHexString<uint8_t>(Items["type"]);		Items.erase("type"); }

					if (Items.count("n")) 		{ Name 		= Items["n"]; 		Items.erase("n"); }
					if (Items.count("name")) 	{ Name 		= Items["name"]; 	Items.erase("name"); }

					if (Items.count("e")) 		{ Extra 	= Converter::UintFromHexString<uint16_t>(Items["e"]);  		Items.erase("e"); }
					if (Items.count("extra")) 	{ Extra 	= Converter::UintFromHexString<uint16_t>(Items["extra"]); 	Items.erase("extra"); }

					if (Items.count("u")) 		{ Updated 	= Converter::ToUint32(Items["u"]); 		Items.erase("u"); }
					if (Items.count("updated")) { Updated 	= Converter::ToUint32(Items["updated"]);Items.erase("updated"); }

					if (Items.count("s")) 		{ Status 	= Converter::UintFromHexString<uint16_t>(Items["s"]); 		Items.erase("s"); }
					if (Items.count("status"))	{ Status 	= Converter::UintFromHexString<uint16_t>(Items["status"]);	Items.erase("status"); }

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
						JSONObject.SetItem((IsShortened) ? "e" : "Extra", Converter::ToHexString(Extra,4));

					if (Status > 0)
						JSONObject.SetItem((IsShortened) ? "s" : "Status", Converter::ToHexString(Status,4));

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

			auto Iterator = IRDevicesList.begin();
			while (Iterator != IRDevicesList.end())
			{
				if (*Iterator == "")
					Iterator = IRDevicesList.erase(Iterator);
				else {
					DataRemote_t::IRDevice DeviceItem(Memory.GetString(*Iterator), *Iterator);
					AddOrUpdateDeviceToCache(DeviceItem);
					++Iterator;
				}
			}
		}

		void AddOrUpdateDeviceToCache(DataRemote_t::IRDevice DeviceItem) {
			if (DeviceItem.IsCorrect()) {
				IRDeviceCacheItem_t CacheItem;

				CacheItem.DeviceID 		= DeviceItem.UUID;
				CacheItem.DeviceType 	= DeviceItem.Type;
				CacheItem.Status		= DeviceItem.Status;
				CacheItem.Extra			= DeviceItem.Extra;

				for (auto& FunctionItem : DeviceItem.Functions)
				{
					uint8_t 	FunctionID 		= DevicesHelper.FunctionIDByName(FunctionItem.first);
					string 		FunctionType 	= FunctionItem.second;

					vector<pair<uint8_t,uint32_t>> FunctionsCache = vector<pair<uint8_t,uint32_t>>();

					for (IRLib &Signal : LoadAllFunctionSignals(DeviceItem.UUID, FunctionItem.first, DeviceItem))
						FunctionsCache.push_back(make_pair(Signal.Protocol, Signal.Uint32Data));

					CacheItem.Functions[FunctionID] = make_pair(FunctionType, FunctionsCache);
				}

				for (int i = 0; i< IRDevicesCache.size(); i++)
					if (IRDevicesCache[i].DeviceID == CacheItem.DeviceID)
					{
						IRDevicesCache[i] = CacheItem;
						return;
					}

				IRDevicesCache.push_back(CacheItem);
			}
		}

		void AddOrUpdateDeviceFunctionInCache(string DeviceUUID, uint8_t FunctionID, string FunctionType, vector<pair<uint8_t, uint32_t>> FunctionData) {
			for (int i=0; i < IRDevicesCache.size(); i++)
				if (IRDevicesCache[i].DeviceID == DeviceUUID) {
					IRDevicesCache[i].Functions[FunctionID] = make_pair(FunctionType, FunctionData);
					return;
				}
		}

		IRDeviceCacheItem_t GetDeviceFromCache(string UUID) {
			for(auto& Item : IRDevicesCache)
				if (Item.DeviceID == UUID)
					return Item;

			IRDeviceCacheItem_t Result;
			return Result;
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
					if (Query.GetParams().count("statuses"))
					{
						vector<string> StatusesArray = vector<string>();

						for (auto& IRDeviceItem : IRDevicesCache)
							StatusesArray.push_back(IRDeviceItem.DeviceID + Converter::ToHexString(IRDeviceItem.Status,4));

						Result.Body = JSON::CreateStringFromVector(StatusesArray);
						Result.ContentType = WebServer_t::Response::TYPE::JSON;
						return;
					}
					else
					{
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
					JSON JSONObject(Query.GetBody());
					map<string,string> Params = JSONObject.GetItems();

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
					JSON JSONObject(Query.GetBody());
					map<string,string> Params = JSONObject.GetItems();

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
					IRDevicesCache.clear();
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

							// delete from cache
							for (int i=0;i<IRDevicesCache.size();i++)
								if (IRDevicesCache[i].DeviceID == DeviceItem.UUID) {
									if (IRDevicesCache[i].Functions.count(DevicesHelper.FunctionIDByName(FunctionToDelete)) > 0) {
										IRDevicesCache[i].Functions.erase(DevicesHelper.FunctionIDByName(FunctionToDelete));
										break;
									}
								}
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

		string RootInfo() override {
			vector<string> StatusesArray = vector<string>();

			for (auto& IRDeviceItem : IRDevicesCache)
				StatusesArray.push_back(IRDeviceItem.DeviceID + Converter::ToHexString(IRDeviceItem.Status,4));

			return JSON::CreateStringFromVector(StatusesArray);
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

			AddOrUpdateDeviceToCache(DeviceItem);

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

			if (!DevicesHelper.IsValidKeyForType(DeviceItem.Type, Function)) {
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

				vector<pair<uint8_t, uint32_t>> FunctionCache = vector<pair<uint8_t, uint32_t>>();

				while(Child) {
					JSON ChildObject(Child);
					ChildObject.SetDestructable(false);

					pair<uint8_t, uint32_t> IRDetails = make_pair(0,0);

					uint8_t SaveFunctionResult = SaveFunction(UUID, Function, SerializeIRSignal(ChildObject, IRDetails), Index++, DeviceItem.Type);

					if (SaveFunctionResult)
					{
						Result.SetFail();
						Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(SaveFunctionResult) + " }";
						return;
					}

					FunctionCache.push_back(IRDetails);

					Child = Child->next;
				}

				AddOrUpdateDeviceFunctionInCache(UUID, DevicesHelper.FunctionIDByName(Function), SignalType, FunctionCache);
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

		pair<bool,uint16_t> SetExternalStatusForAC(uint16_t Codeset, uint16_t NewStatus) {
			uint16_t 	Status 		= 0x0;
			string 		DeviceID	= "";

			for (auto& IRDeviceCacheItem : IRDevicesCache)
				if (IRDeviceCacheItem.DeviceType == 0xEF && IRDeviceCacheItem.Extra == Codeset)
				{
					Status 		= IRDeviceCacheItem.Status;
					DeviceID	= IRDeviceCacheItem.DeviceID;
					break;
				}

			if (DeviceID == "")
				return make_pair(false, Status);

			ACOperand ACOperandPrev((uint32_t)Status);
			ACOperand ACOperandNext((uint32_t)NewStatus);

			StatusSave(DeviceID, NewStatus);

#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
			if (ACOperandPrev.Mode 			!= ACOperandNext.Mode) 			HomeKitStatusTriggerUpdated(DeviceID, 0xEF, 0xE0, ACOperandNext.Mode);
			if (ACOperandPrev.Temperature 	!= ACOperandNext.Temperature) 	HomeKitStatusTriggerUpdated(DeviceID, 0xEF, 0xE1, ACOperandNext.Temperature);
			if (ACOperandPrev.FanMode 		!= ACOperandNext.FanMode) 		HomeKitStatusTriggerUpdated(DeviceID, 0xEF, 0xE2, ACOperandNext.FanMode);
			if (ACOperandPrev.SwingMode 	!= ACOperandNext.SwingMode) 	HomeKitStatusTriggerUpdated(DeviceID, 0xEF, 0xE3, ACOperandNext.SwingMode);
#endif

			StatusTriggerUpdated(DeviceID, 0xEF, 0, 0, NewStatus);

			return make_pair(true, NewStatus);
		}

		bool SetExternalStatusByIRCommand(IRLib &Signal) {
			for (auto& CacheItem : IRDevicesCache) {
				if (CacheItem.DeviceType == 0xEF) continue;

				for (auto& FunctionItem : CacheItem.Functions) {
					uint8_t FunctionID 		= FunctionItem.first;
					string 	FunctionType	= FunctionItem.second.first;

					for (int i = 0 ; i< FunctionItem.second.second.size(); i++) {
						pair<uint8_t, uint32_t> FunctionSignal = FunctionItem.second.second[i];

						if (Signal.Protocol == FunctionSignal.first) { // protocol identical
							if (Signal.Protocol == 0xFF) { // raw signal
								if (LoadFunctionByIndex(CacheItem.DeviceID, DevicesHelper.FunctionNameByID(FunctionID), i).second.CompareIsIdenticalWith(Signal))
								{
									StatusUpdateForDevice(CacheItem.DeviceID, FunctionID, i);
									break;
								}
							}
							else if (Signal.Uint32Data == FunctionSignal.second && Signal.Uint32Data != 0x0)
							{
								StatusUpdateForDevice(CacheItem.DeviceID, FunctionID, i);
								break;
							}
						}
					}
				}
			}

			return false;
		}

		pair<bool,uint16_t> StatusUpdateForDevice(string DeviceID, uint8_t FunctionID, uint8_t Value, string FunctionType = "", bool IsBroadcasted = true) {
			uint16_t 	Status 		= 0x0;
			uint8_t		DeviceType	= 0x0;

			for (auto& IRDeviceCacheItem : IRDevicesCache)
				if (IRDeviceCacheItem.DeviceID == DeviceID)
				{
					Status 		= IRDeviceCacheItem.Status;
					DeviceType 	= IRDeviceCacheItem.DeviceType;
					break;
				}

			if (DeviceType == 0x0) return make_pair(false, Status);

			pair<uint16_t,uint8_t> UpdateStatusResult = DevicesHelper.GetDeviceForType(DeviceType)->UpdateStatusForFunction(Status, FunctionID, Value, FunctionType);
			uint16_t NewStatus = UpdateStatusResult.first;
			Value = UpdateStatusResult.second;

			ESP_LOGE("OLD STATUS", "%04X", Status);
			ESP_LOGE("NEW STATUS", "%04X", NewStatus);

			if (Status == NewStatus)
				return make_pair(false, Status);

			StatusSave(DeviceID, NewStatus);

			if (IsBroadcasted)
				StatusTriggerUpdated(DeviceID, DeviceType, FunctionID, Value, NewStatus);

			return make_pair(true, NewStatus);
		}

		void StatusSave(string DeviceID, uint16_t Status) {
			for (int i = 0; i< IRDevicesCache.size(); i++)
				if (IRDevicesCache[i].DeviceID == DeviceID)
				{
					IRDevicesCache[i].Status = Status;

					IRDevice DeviceItem = LoadDevice(DeviceID);
					DeviceItem.Status = Status;
					SaveDevice(DeviceItem);

					break;
				}
		}

		void StatusTriggerUpdated(string DeviceID, uint8_t DeviceType, uint8_t FunctionID, uint8_t Value, uint16_t Status) {
#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
			if (FunctionID > 0)
				HomeKitStatusTriggerUpdated(DeviceID, DeviceType, FunctionID, Value);
#endif
			Wireless.SendBroadcastUpdated(0x87, "FE", DeviceID + Converter::ToHexString(Status,4));

		}

#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
		void HomeKitStatusTriggerUpdated(string DeviceID, uint8_t DeviceType, uint8_t FunctionID, uint8_t Value) {
			ESP_LOGE("StatusTriggerUpdated", "FunctionID: %04x Value: %d", FunctionID, Value);

			uint32_t AID = (uint32_t)Converter::UintFromHexString<uint16_t>(DeviceID);

			static hap_val_t HAPValue;

			switch (FunctionID) {
				case 0x01:
				case 0x02:
				case 0x03:
					HAPValue.u = Value;

					switch (DeviceType) {
						case 0x01:
							UpdateHomeKitCharValue(AID, SERVICE_TV_UUID, HAP_CHAR_UUID_ACTIVE, HAPValue);
							UpdateHomeKitCharValue(AID, SERVICE_TV_UUID, CHAR_SLEEP_DISCOVERY_UUID, HAPValue);
							break;
						case 0x07:
							UpdateHomeKitCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ACTIVE, HAPValue);
							break;
						default:
							break;
					}

					break;
				case 0x04:
					if (DeviceType == 0x01) { // TV, input mode
						HAPValue.u = (Value + 1);
						UpdateHomeKitCharValue(AID, SERVICE_TV_UUID, CHAR_ACTIVE_IDENTIFIER_UUID, HAPValue);
					}
					break;

				case 0x05: // mute
					HAPValue.b = (Value == 0) ? true : false;
					UpdateHomeKitCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_MUTE_UUID, HAPValue);

					static hap_val_t HAPValueVolumeControlType;
					HAPValueVolumeControlType.u = (Value == 0) ? 1 : 0;
					UpdateHomeKitCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_VOLUME_CONTROL_TYPE_UUID, HAPValueVolumeControlType);
					break;
				case 0x06: // volume up
				{
					HAPValue.b = false;
					UpdateHomeKitCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_MUTE_UUID, HAPValue);

					static hap_val_t HAPValueVolumeControlType;
					HAPValueVolumeControlType.u = 1;
					UpdateHomeKitCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_VOLUME_CONTROL_TYPE_UUID, HAPValueVolumeControlType);

					break;
				}
				case 0x07: // volume down
				{
					HAPValue.b = false;
					UpdateHomeKitCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_MUTE_UUID, HAPValue);

					static hap_val_t HAPValueVolumeControlType;
					HAPValueVolumeControlType.u = 1;
					UpdateHomeKitCharValue(AID, SERVICE_TELEVISION_SPEAKER_UUID, CHAR_VOLUME_CONTROL_TYPE_UUID, HAPValueVolumeControlType);
					break;
				}
				case 0xE0: { // AC mode
					HAPValue.u = (Value > 0) ? 3 : 0;
					ESP_LOGE("HAPValue.u", "%d", HAPValue.u);

					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_HEATER_COOLER_STATE, HAPValue);

					HAPValue.b = (Value > 0);
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_HEATER_COOLER	, HAP_CHAR_UUID_ACTIVE, HAPValue);
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_FAN_V2		, HAP_CHAR_UUID_ACTIVE, HAPValue);


					static hap_val_t HAPValueFanRotation;
					static hap_val_t HAPValueFanAuto;

			        DataRemote_t::IRDeviceCacheItem_t IRDeviceItem = GetDeviceFromCache(DeviceID);
					uint8_t FanStatus = DataDeviceItem_t::GetStatusByte(IRDeviceItem.Status, 2);

					if (FanStatus > 3) FanStatus = 3;

					HAPValueFanRotation.f 	= (Value > 0) ? ((FanStatus > 0) ? FanStatus : 2) : 0;
					HAPValueFanAuto.u 		= (Value > 0) ? ((FanStatus == 0) ? 1 : 0) : 0;

					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_ROTATION_SPEED, HAPValueFanRotation);
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_FAN_V2, HAP_CHAR_UUID_TARGET_FAN_STATE, HAPValueFanAuto);
					break;
				}
				case 0xE1: // AC Temeperature
					HAPValue.f = (float)Value;
					//UpdateHomeKitCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_CURRENT_TEMPERATURE, HAPValue);
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_COOLING_THRESHOLD_TEMPERATURE, HAPValue);
					break;
				case 0xE2: // Fan Mode
					HAPValue.f = (float) ((Value > 0) ? Value: 0);
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_FAN_V2		, HAP_CHAR_UUID_ROTATION_SPEED, HAPValue);

					HAPValue.u = (Value > 0) ? 0 : 1;
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_FAN_V2		, HAP_CHAR_UUID_TARGET_FAN_STATE, HAPValue);

					break;
				case 0xE3: // Swing Mode
					HAPValue.u = Value;
					UpdateHomeKitCharValue(AID, HAP_SERV_UUID_HEATER_COOLER, HAP_CHAR_UUID_SWING_MODE, HAPValue);

					break;
			}


			/*
			 *
			if (ACOperandPrev.FanMode 		!= ACOperandNext.FanMode) 		StatusTriggerUpdated(DeviceID, 0xEF, 0xE2, ACOperandNext.FanMode);
			if (ACOperandPrev.SwingMode 	!= ACOperandNext.SwingMode) 	StatusTriggerUpdated(DeviceID, 0xEF, 0xE3, ACOperandNext.SwingMode);
			 *
			 */


		}


		void UpdateHomeKitCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID, hap_val_t Value) {
			hap_acc_t* Accessory 	= hap_acc_get_by_aid(AID);
			if (Accessory == NULL) 	return;

			ESP_LOGE("1","!");
			hap_serv_t *Service  	= hap_acc_get_serv_by_uuid(Accessory, ServiceUUID);
			if (Service == NULL) 	return;
			ESP_LOGE("2","!");
			hap_char_t *Char 		= hap_serv_get_char_by_uuid(Service, CharUUID);
			if (Char == NULL) 		return;
			ESP_LOGE("3","!");
			int ttt = hap_char_update_val(Char, &Value);
			ESP_LOGE("hap_char_update_val", "%d", ttt);
		}
#endif

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

				auto it = IRDevicesCache.begin();
				while (it != IRDevicesCache.end())
				{
					if ((*it).DeviceID == UUID)
						it = IRDevicesCache.erase(it);
					else
						++it;
				}

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

				if (!DevicesHelper.IsValidKeyForType(DeviceType, Function))
					return static_cast<uint8_t>(DataRemote_t::Error::UnsupportedFunction);

				string ValueName = UUID + "_" + Function;

				if (Index > 0)
					ValueName += "_" + Converter::ToString(Index);

				Memory.SetString(ValueName, Item);

				return 0;
			}

			string SerializeIRSignal(JSON &JSONObject, pair<uint8_t,uint32_t> &IRDetails) {
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

				if (Signal.Protocol != 0xFF) {
					IRDetails 	= make_pair(Signal.Protocol, Signal.Uint32Data);
					return Converter::ToHexString(Signal.Protocol,2) + Converter::ToHexString(Signal.Uint32Data, 8);
				}
				else if (Signal.Protocol == 0xFF && Signal.RawData.size() > 0) {
					IRDetails 	= make_pair(0xFF, 0);
					return Signal.GetProntoHex(false);
				}
				else {
					IRDetails 	= make_pair(0, 0);
					return "";
				}
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

			void DebugIRDevicesCache() {
				for (auto& Item : IRDevicesCache) {
					ESP_LOGE("DeviceID"		, "%s"	, Item.DeviceID.c_str());
					ESP_LOGE("DeviceType"	, "%02X", Item.DeviceType);
					ESP_LOGE("Status"		, "%04X", Item.Status);
					ESP_LOGE("Extra"		, "%04X", Item.Extra);

					ESP_LOGE("Functions"	, " ");
					for (auto& MapItem : Item.Functions) {
						ESP_LOGE("-->"		, "%02X %s"	, MapItem.first, MapItem.second.first.c_str());

						for (auto& MapFunctionItem : MapItem.second.second)
							ESP_LOGE("---->", "%02X %02X"	, MapFunctionItem.first, MapFunctionItem.second);
					}
				}
			}
};

#endif

