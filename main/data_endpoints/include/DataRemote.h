/*
*    DataRemote.cpp
*    Class to handle API /Data for Remote Device
*
*/

#ifndef DATA_REMOTE_H
#define DATA_REMOTE_H

#include "Data.h"

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <stdio.h>
#include <string.h>

#include <esp_netif_ip_addr.h>

#include "Webserver.h"
#include "WiFi.h"
#include "Wireless.h"

#include "esp_timer.h"

#include <IRLib.h>

using namespace std;

class DataDeviceItem_t {
	public:
		uint8_t DeviceTypeID = 0;
		virtual vector<uint8_t> GetAvaliableFunctions() { return vector<uint8_t>(); }
		virtual pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value,  string FunctionType = "") {
			return make_pair(Status, Value);
		}

		virtual uint16_t LastStatusUpdate(uint16_t SavedLastStatus, uint16_t CurrentStatus, uint16_t NewStatus) {
			return CurrentStatus;
		}

		virtual ~DataDeviceItem_t() {};

		static uint8_t GetStatusByte(uint16_t Status, uint8_t ByteID);
		static uint16_t SetStatusByte(uint16_t Status, uint8_t ByteID, uint8_t Value);

		pair<bool,uint8_t> CheckPowerUpdated(uint8_t FunctionID, uint8_t Value, uint8_t CurrentValue = 0, string FunctionType = "");
};

class DataDeviceTV_t  : public DataDeviceItem_t {
	public:
		DataDeviceTV_t();

		vector<uint8_t> GetAvaliableFunctions() override;		
		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;

		virtual ~DataDeviceTV_t() {};
};

class DataDeviceMedia_t  : public DataDeviceItem_t {
	public:
		DataDeviceMedia_t();

		vector<uint8_t> GetAvaliableFunctions() override;		
		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;

		virtual ~DataDeviceMedia_t() {};
};

class DataDeviceLight_t  : public DataDeviceItem_t {
	public:
		DataDeviceLight_t();

		vector<uint8_t> GetAvaliableFunctions() override;
		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;

		virtual ~DataDeviceLight_t() {};
};

class DataDeviceHumidifier_t  : public DataDeviceItem_t {
	public:
		DataDeviceHumidifier_t();

		vector<uint8_t> GetAvaliableFunctions() override;
		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;

		virtual ~DataDeviceHumidifier_t() {};
};

class DataDeviceAirPurifier_t  : public DataDeviceItem_t {
	public:
		DataDeviceAirPurifier_t();

		vector<uint8_t> GetAvaliableFunctions() override;
		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;

		virtual ~DataDeviceAirPurifier_t() {};
};

class DataDeviceRoboCleaner_t  : public DataDeviceItem_t {
	public:
		DataDeviceRoboCleaner_t();

		vector<uint8_t> GetAvaliableFunctions() override;
		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;

		virtual ~DataDeviceRoboCleaner_t() {};
};

class DataDeviceFan_t  : public DataDeviceItem_t {
	public:
		DataDeviceFan_t();
		vector<uint8_t> GetAvaliableFunctions() override;

		pair<uint16_t, uint8_t> UpdateStatusForFunction(uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;
		
		virtual ~DataDeviceFan_t() {};
};

class DataDeviceAC_t : public DataDeviceItem_t {
	public:
		DataDeviceAC_t();

		pair<uint16_t, uint8_t> UpdateStatusForFunction (uint16_t Status, uint8_t FunctionID, uint8_t Value, string FunctionType = "") override;
		uint16_t LastStatusUpdate(uint16_t SavedLastStatus, uint16_t CurrentStatus, uint16_t NewStatus) override;

		virtual ~DataDeviceAC_t() {};
};

class DataDevice_t {
	public:
		DataDevice_t();
		~DataDevice_t();
		
		uint8_t FunctionIDByName(string FunctionName);
		string FunctionNameByID(uint8_t FunctionID);

		DataDeviceItem_t* GetDeviceForType(uint8_t Type);

		bool IsValidKeyForType(uint8_t Type, string Key);

	private:
		vector<DataDeviceItem_t *> 	DevicesClasses = vector<DataDeviceItem_t *>();

		map<string, uint8_t> GlobalFunctions =
		{
			{ "power"		, 0x01 }, { "poweron"	, 0x02 }, { "poweroff"	, 0x03 	},
			{ "mode"		, 0x04 },
			{ "mute"		, 0x05 }, { "volup"		, 0x06 }, { "voldown"	, 0x07 	},
			{ "chup"		, 0x08 }, { "chdown"	, 0x09 },
			{ "swing"		, 0x0A }, { "speed"		, 0x0B },
			{ "cursor"		, 0x0C },
			{ "menu"		, 0x0D },
			{ "back"		, 0x0E },
			{ "play"		, 0x0F },
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

		enum SignalType {
			IR					= 0x00,
			BLE					= 0x01,
			Empty				= 0xFF
		};

		struct DataSignal {
			SignalType	Type 			{ Empty };
			IRLib		IRSignal		{ };
			string		BLESignal		{ };
		};

		const string ResponseItemDidntExist 	= "{\"success\" : \"false\", \"Message\" : \"Item didn't exists\" }";
		const string ResponseItemAlreadyExist 	= "{\"success\" : \"false\", \"Message\" : \"Item already exists\" }";

		struct IRDeviceCacheItem_t {
			string 			DeviceID 	{ ""  };
			uint8_t			DeviceType 	{ 0x0 };
			uint16_t		Status 		{ 0x0 };
			uint16_t		LastStatus	{ 0x0 };
			uint16_t		Extra		{ 0x0 };
			uint32_t		Updated		{ 0x0 };

			bool			StatusSaved	{ true };

			map<uint8_t, pair<string,vector<tuple<uint8_t,uint32_t, uint16_t>>>>
							Functions = map<uint8_t, pair<string,vector<tuple<uint8_t,uint32_t, uint16_t>>>>();

			bool IsEmpty() { return (DeviceID == "" && DeviceType == 0x0); }
		};

		vector<IRDeviceCacheItem_t> IRDevicesCache = vector<IRDeviceCacheItem_t>();

		class IRDevice {
			public:
				string 		UUID 		= "";
				uint8_t 	Type 		= 0xFF;
				uint32_t	Updated 	= 0;
				string 		Name		= "";
				uint16_t	Extra 		= 0;
				uint16_t	Status		= 0;
				uint16_t	LastStatus 	= 0;

				map<string,string> Functions = map<string,string>();

				void ParseJSONItems(map<string, string> Items);

				IRDevice(string Data = "", string sUUID = "");

				IRDevice(map<string,string> Items, string sUUID = "");

				bool IsCorrect();

				string ToString(bool IsShortened = true);
		};

		void Init() override;

		void InitDevicesCacheFromNVSKeys();

		void RemoveDeviceFromCache(string UUID);

		pair<uint16_t,uint16_t> GetStatusPair(uint8_t Type, uint16_t Extra);

		void AddOrUpdateDeviceToCache(DataRemote_t::IRDevice DeviceItem);
		void AddOrUpdateDeviceFunctionInCache(string DeviceUUID, uint8_t FunctionID, string FunctionType, vector<tuple<uint8_t, uint32_t, uint16_t>> FunctionData);

		IRDeviceCacheItem_t GetDeviceFromCache(string UUID);

		void ClearChannels(vector<gpio_num_t> &GPIOs);

		void InnerHTTPRequest(WebServer_t::Response &Result, Query_t &Query) override;

		string RootInfo() override;

		void PUTorPOSTDeviceItem(Query_t &Query, string UUID, WebServer_t::Response &Result);
		void PUTorPOSTDeviceFunction(Query_t &Query, string UUID, string Function, WebServer_t::Response &Result);

		map<string,string> LoadDeviceFunctions(string UUID);
		DataSignal LoadFunctionByIndex(string UUID, string Function, uint8_t Index = 0x0, IRDevice DeviceItem = IRDevice());
		vector<DataSignal> LoadAllFunctionSignals(string UUID, string Function, IRDevice DeviceItem = IRDevice());
		string GetFunctionType(string UUID, string Function, IRDevice DeviceItem = IRDevice());

		string GetUUIDByExtraValue(uint16_t Extra);

		pair<bool,uint16_t> SetExternalStatusForAC(uint16_t Codeset, uint16_t NewStatus);
		bool 				SetExternalStatusByIRCommand(IRLib &Signal);
		pair<bool,uint16_t> StatusUpdateForDevice(string DeviceID, uint8_t FunctionID, uint8_t Value, string FunctionType = "", bool IsBroadcasted = true, bool EmulateSave = false);

		void StatusSave(string DeviceID, uint16_t StatusToSave);
		static void StatusSaveCallback(void *Param);

		IRDevice GetDevice(string UUID);

		void StatusTriggerUpdated(string DeviceID, uint8_t DeviceType, uint8_t FunctionID, uint8_t Value, uint16_t Status);
		void HomeKitStatusTriggerUpdated(string DeviceID, uint8_t DeviceType, uint8_t FunctionID, uint8_t Value);

		private:
			static inline esp_timer_handle_t StatusSaveTimer = NULL;

			map<string,vector<string>> 	AvaliableFunctions 	= map<string,vector<string>>();

			IRDevice LoadDevice(string UUID);
			uint8_t	SaveDevice(IRDevice DeviceItem);

			bool SaveFunction(string UUID, string Function, string Item, uint8_t Index, uint8_t DeviceType);

			string SerializeSignal(JSON &JSONObject, std::tuple<uint8_t,uint32_t, uint16_t> &IRDetails);
			DataSignal DeserializeSignal(string Item);

			void DebugIRDevicesCache();
};

#endif

