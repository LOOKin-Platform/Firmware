/*
*    Network.cpp
*    Class to handle API /Device
*
*/
#include "Globals.h"
#include "Data.h"

static char 	Tag[] 				= "Data_t";
static string 	NVSDataArea 		= "Data";
vector<string>	Data::IRDevicesList	= vector<string>();

void Data::Init() {
	NVS Memory(NVSDataArea);

	IRDevicesList = Converter::StringToVector(Memory.GetString("DevicesList"), "|");

	for (int i = 0; i< IRDevicesList.size(); i++)
		if (IRDevicesList[i] == "") IRDevicesList.erase(IRDevicesList.begin() + i);
}

void Data::IRDevice::ParseJSONItems(map<string, string> Items) {
	if (Items.size() == 0)
		return;

	if (Items.count("t")) 		{ Type 		= Items["t"]; 		Items.erase("t"); }
	if (Items.count("type")) 	{ Type 		= Items["type"]; 	Items.erase("type"); }

	if (Items.count("n")) 		{ Name 		= Items["n"]; 		Items.erase("n"); }
	if (Items.count("name")) 	{ Name 		= Items["name"]; 	Items.erase("name"); }

	if (Items.count("u")) 		{ Updated 	= Converter::ToUint32(Items["u"]); Items.erase("u"); }
	if (Items.count("updated")) { Updated 	= Converter::ToUint32(Items["updated"]); Items.erase("updated"); }

	if (Items.count("uuid")) 	{ UUID		= Items["uuid"]; 	Items.erase("uuid"); }

	for (auto& Item : Items)
		Functions[Item.first] = Item.second;

	Converter::FindAndRemove(UUID, "|");
}

Data::IRDevice::IRDevice(string sInput, string sUUID) {
	if (sUUID != "") 	UUID = sUUID;
	if (sInput == "") 	return;

	JSON JSONObject(sInput);
	ParseJSONItems(JSONObject.GetItems());
}

Data::IRDevice::IRDevice(map<string,string> Items, string sUUID) {
	if (sUUID != "")	UUID = sUUID;
	ParseJSONItems(Items);
}


bool Data::IRDevice::IsCorrect() {
	if (UUID == "") return false;

	if (Converter::ToLower(Type) == "tv") return true;

	return false;
}

string Data::IRDevice::ToString(bool IsShortened) {
	JSON JSONObject;

	JSONObject.SetItem((IsShortened) ? "t" : "Type", Type);
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

void Data::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody, httpd_req_t *Request,  WebServer_t::QueryTransportType TransportType, int MsgID) {
	if (URLParts.size() == 1)
		if (Converter::ToLower(URLParts[0]) == "deviceslist") // Forbidden to use this as UUID
		{
			Result.SetFail();
			return;
		}

	if (Type == QueryType::GET) {
		if (URLParts.size() == 0) {

			NVS Memory(NVSDataArea);
			vector<map<string,string>> OutputArray = vector<map<string,string>>();

			for (auto& IRDeviceUUID: IRDevicesList) {
				Data::IRDevice DeviceItem(Memory.GetString(IRDeviceUUID), IRDeviceUUID);

				//OutputArray.push_back({{"UUID", DeviceItem.UUID}});

				OutputArray.push_back({
					{ "UUID" 	, 	DeviceItem.UUID							},
					{ "Type"	, 	DeviceItem.Type 						},
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
			Data::IRDevice DeviceItem = LoadDevice(URLParts[0]);

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
			string UUID 	= URLParts[0];
			string Function	= URLParts[1];

			JSON JSONObject;

			Data::IRDevice DeviceItem = LoadDevice(UUID);

			if (!DeviceItem.IsCorrect()) {
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(Data::Error::DeviceNotFound)) + " }";
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
			Data::IRDevice NewDeviceItem;

			string UUID = "";
			if (Params.count("uuid")) UUID = Params["uuid"];
			if (URLParts.size() == 1) UUID = URLParts[0];

			if (UUID == "")
			{
				Result.SetFail();
				return;
			}

			Data::IRDevice DeviceItem = LoadDevice(UUID);

			if (DeviceItem.IsCorrect())
				DeviceItem.ParseJSONItems(Params);
			else
				DeviceItem = Data::IRDevice(Params, UUID);

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
			string UUID 	= URLParts[0];
			string Function	= URLParts[1];

			JSON JSONObject(RequestBody);

			Data::IRDevice DeviceItem = LoadDevice(UUID);

			if (!DeviceItem.IsCorrect()) {
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(Data::Error::DeviceNotFound)) + " }";
				return;
			}

			if (!JSONObject.IsItemExists("signals"))
			{
				Result.SetFail();
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(Data::Error::SignalsFieldEmpty)) + " }";
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
				Result.Body = "{\"success\" : \"false\", \"Code\" : " + Converter::ToString(static_cast<uint8_t>(Data::Error::SignalsFieldEmpty)) + " }";
				return;
			}
		}
	}

	if (Type == QueryType::DELETE) {
		if (URLParts.size() == 1)
		{
			NVS Memory(NVSDataArea);

			RemoveFromIRDevicesList(URLParts[0]);
			Memory.EraseStartedWith(URLParts[0]);
		}
	}

	Result.SetSuccess();
}

Data::IRDevice Data::LoadDevice(string UUID) {
	NVS Memory(NVSDataArea);

	IRDevice Result(Memory.GetString(UUID), UUID);

	return Result;
}

uint8_t Data::SaveIRDevicesList() {
	NVS Memory(NVSDataArea);
	return (Memory.SetString("DevicesList", Converter::VectorToString(IRDevicesList, "|"))) ? 0 : static_cast<uint8_t>(NoSpaceToSaveDevice);
}

uint8_t Data::AddToDevicesList(string UUID) {
	if (!(std::count(IRDevicesList.begin(), IRDevicesList.end(), UUID)))
	{
		IRDevicesList.push_back(UUID);
		uint8_t SetResult = SaveIRDevicesList();

		return (SetResult != 0) ? SetResult + 0x10 : 0;
	}

	return 0;//static_cast<uint8_t>(Error::DeviceAlreadyExists);
}

uint8_t Data::RemoveFromIRDevicesList(string UUID) {
	std::vector<string>::iterator itr = std::find(IRDevicesList.begin(), IRDevicesList.end(), UUID);
	if (itr != IRDevicesList.end()) IRDevicesList.erase(itr);

	return SaveIRDevicesList();
}


uint8_t Data::SaveDevice(Data::IRDevice DeviceItem) {
	NVS Memory(NVSDataArea);

	uint8_t SetResult = Memory.SetString(DeviceItem.UUID, DeviceItem.ToString());

	if (SetResult != 0) return SetResult;

	return AddToDevicesList(DeviceItem.UUID);
}

vector<IRLib> Data::LoadAllFunctionSignals(string UUID, string Function, Data::IRDevice DeviceItem) {
	UUID 		= Converter::ToLower(UUID);
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

pair<bool,IRLib> Data::LoadFunctionByIndex(string UUID, string Function, uint8_t Index, IRDevice DeviceItem) {
	UUID 	= Converter::ToLower(UUID);
	Function= Converter::ToLower(Function);

	NVS Memory(NVSDataArea);

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

uint8_t Data::SaveFunction(string UUID, string Function, string Item, uint8_t Index, string DeviceType) {
	NVS Memory(NVSDataArea);

	if (!CheckIsValidKeyForType(DeviceType, Function))
		return static_cast<uint8_t>(Data::Error::UnsupportedFunction);

	string ValueName = UUID + "_" + Function;

	if (Index > 0)
		ValueName += "_" + Converter::ToString(Index);

	Memory.SetString(ValueName, Item);

	return 0;
}

string Data::SerializeIRSignal(JSON &JSONObject) {
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

pair<bool,IRLib> Data::DeserializeIRSignal(string Item) {
	IRLib Result;

	if (Item == "")
		return make_pair(false, Result);

	if (Item.size() == 10)
	{
		Result.Protocol 	= Converter::ToUint8(Item.substr(0, 2));
		Result.Uint32Data	= Converter::ToUint32(Item.substr(2));
	}

	return make_pair(true,IRLib(Item));
}

bool Data::CheckIsValidKeyForType(string Type, string Key) {
	vector<string> AvaliableKeys = vector<string>();

	Type = Converter::ToLower(Type);

	if (Type == "tv") {
		AvaliableKeys.push_back("power");		// power
		AvaliableKeys.push_back("volumeup");	// volume up
		AvaliableKeys.push_back("volumedown");	// volume down
		AvaliableKeys.push_back("channelup");	// channel up
		AvaliableKeys.push_back("channeldown");	// channel down
	}

	if(std::find(AvaliableKeys.begin(), AvaliableKeys.end(), Converter::ToLower(Key)) != AvaliableKeys.end())
	    return true;
	else
		return false;
}
