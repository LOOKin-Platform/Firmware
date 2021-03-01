/*
*    Storage.cpp
*    Class for handling storage functions
*
*/

#include "Storage.h"
#include "Globals.h"

const char tag[] = "Storage";

void Storage_t::Init() {
	uint32_t VersionsStart	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB 	:  Settings.Storage.Versions.StartAddress16MB;
	uint32_t VersionsSize	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.Size4MB 			:  Settings.Storage.Versions.Size16MB;

	AddressToWrite = VersionsStart;

	while (AddressToWrite < VersionsStart + VersionsSize) {
		uint16_t FindedID = SPIFlash::ReadUint16(AddressToWrite);

		if (FindedID == Settings.Memory.Empty16Bit)
			break;

		if (FindedID > LastVersion)
			LastVersion = FindedID;

		AddressToWrite += 2;
	}

	CalculateMemoryItemsData();
}

void Storage_t::Item::SetData(string StringData) {
	Data.clear();

	if (StringData.size()%2 == 1) StringData += "0";

	int pos = 0;
	while (pos!=StringData.size()) {
		Data.push_back(Converter::UintFromHexString<uint8_t>(StringData.substr(pos, (StringData.size() - pos > 2) ? 2 : StringData.size() - pos)));
		pos += (StringData.size() - pos > 2) ? 2 : StringData.size() - pos;
	}

	CalculateLength();
}

void Storage_t::Item::SetData(uint8_t ByteData) {
	Data.push_back(ByteData);

	CalculateLength();
}

void Storage_t::Item::SetData(vector<uint8_t> VectorData) {
	Data = VectorData;

	CalculateLength();
}

vector<uint8_t> Storage_t::Item::GetData() {
	return Data;
}

string Storage_t::Item::DataToString() {
	string Result = "";

	for (auto &DataItem : Data)
		Result += Converter::ToHexString(DataItem, 2);

	return Result;
}

uint16_t Storage_t::Item::CRC16ForData(uint16_t Start, uint16_t Length) {
	return Converter::CRC16(Data, Start, Length);
}


void Storage_t::Item::ClearData() {
	Data.clear();
	Header.Length = 0;
}

void Storage_t::Item::CalculateLength() {
	Header.Length = (uint8_t)ceil((float)Data.size() / (float)Storage_t::RecordDataSize());
}

Storage_t::Item Storage_t::Read(uint16_t ItemID) {
	uint32_t DataStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	uint32_t 			Address 		= DataStart;
	vector<uint32_t> 	AddressVector 	= vector<uint32_t>();
	Storage_t::Item 	Result;

	if (ItemID > DataSize / Settings.Storage.Data.ItemSize)
		return Result;

	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger 	= SPIFlash::ReadUint32(Address);

		if (Header.MemoryID == ItemID) {
			AddressVector.push_back(Address);
			if (Header.Length == 0x0) break;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	if (Address > DataStart + DataSize)
		return Result;

	if (AddressVector.size() > 0)
		for (int i=0; i < AddressVector.size(); i++) {
			RecordHeader Header;
			Header.HeaderAsInteger = SPIFlash::ReadUint32(AddressVector[i]);

			RecordPartInfo PartInfo;
			PartInfo.PartInfoAsInteger = SPIFlash::ReadUint32(AddressVector[i] + 0x4);

			if (i == 0) Result.Header.HeaderAsInteger = Header.HeaderAsInteger;

			for (uint16_t j=0; j < 4 * ceil((float)(PartInfo.Size+1) / (float)4); j+=4) {
				uint32_t Uint32Data = SPIFlash::ReadUint32LE(AddressVector[i] + RecordHeaderSize() + j);

				for (uint8_t k = 0; k < ((j+4 <= (PartInfo.Size + 1)) ? 4 : PartInfo.Size + 1 - j); k++) {
					Result.SetData((uint8_t)Uint32Data);
					Uint32Data = Uint32Data >> 8;
				}
			}

			if (Header.Length == 0x0)
				break;
		}

	return Result;
}

uint16_t Storage_t::Write(Item Record) {
	uint32_t DataSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	if (Record.Header.MemoryID== 0xFFFF)
		Record.Header.MemoryID = FindFreeID();

	if (Record.Header.MemoryID == 0xFFFF)
		return 0xFFFF;

	if (Record.Header.MemoryID >= DataSize / Settings.Storage.Data.ItemSize - 1)
		Record.Header.MemoryID = DataSize / Settings.Storage.Data.ItemSize - 1;

	vector<Item>::iterator it = Patch->begin();
	// delete write items already in commit
	while (it != Patch->end())
		if (it->Header.MemoryID == Record.Header.MemoryID)
			it = Patch->erase(it);
		else
		    ++it;

	// if ID exist erase before writing
	if (IsExist(Record.Header.MemoryID)) {
		Item ItemToErase = Record;
		ItemToErase.ClearData();
		Patch->push_back(ItemToErase);
	}

	Patch->push_back(Record);

	return Record.Header.MemoryID;
}

void Storage_t::Erase(uint16_t ItemID) {
	Item ErasedItem;
	ErasedItem.Header.MemoryID = ItemID;
	ErasedItem.Header.Length = 0;

	// delete write and erase items already in commit
	vector<Item>::iterator it = Patch->begin();
	while (it != Patch->end())
		if (it->Header.MemoryID == ItemID)
			it = Patch->erase(it);
		else
		    ++it;

	Patch->push_back(ErasedItem);
}

void Storage_t::Commit(uint16_t Version) {
	while (Patch->size() > 0) {
		uint8_t Size = 0;

		if (Version <= LastVersion) Version = 0;


		vector<pair<uint16_t,uint8_t>> Items = {};

		while (Patch->front().Header.Length + Size < Settings.Storage.Versions.VersionMaxSize && Patch->size() > 0) {
			Size += (Patch->front().Header.Length > 0) ? Patch->front().Header.Length : 1 ;

			if (Patch->front().Header.Length == 0)
			{
				EraseNow(Patch->front().Header.MemoryID);

				uint16_t NewMemoryID = Patch->front().Header.MemoryID + 0x1000;

				// delete already added items with same MemoryID
				vector<pair<uint16_t,uint8_t>>::iterator it = Items.begin();
				while (it != Items.end())
					if (it->first == NewMemoryID)
						it = Items.erase(it);
					else
					    ++it;

				Items.push_back(make_pair(NewMemoryID, (uint8_t)Patch->front().Header.TypeID));
			}
			else
			{
				WriteNow(Patch->front());

				// delete already added items with same MemoryID
				vector<pair<uint16_t,uint8_t>>::iterator it = Items.begin();
				while (it != Items.end())
					if (it->first == Patch->front().Header.MemoryID)
						it = Items.erase(it);
					else
					    ++it;

				Items.push_back(make_pair((uint16_t)Patch->front().Header.MemoryID, (uint8_t)Patch->front().Header.TypeID));
			}
			Patch->erase(Patch->begin());
		}

		LastVersion = (Version > 0) ? Version : LastVersion + 1;

		SPIFlash::WriteUint16(LastVersion, AddressToWrite);
		AddressToWrite += 0x2;

		for (auto const&Item : Items) {
			SPIFlash::WriteUint16(Item.first, AddressToWrite);
			AddressToWrite+=2;
		}
	}
}

uint16_t Storage_t::CurrentVersion() {
	return LastVersion;
}


void Storage_t::HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	uint32_t VersionsStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;
	uint32_t VersionsSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.Size4MB : Settings.Storage.Versions.Size16MB;

	if (Query.Type == QueryType::GET) {
		if (Query.GetURLPartsCount() == 1) {
			JSON JSONObject;

			JSONObject.SetItem("Version" , VersionToString());
			JSONObject.SetItem("Count"	 , Converter::ToString(MemoryStoredItems));
			JSONObject.SetItem("Freesize", Converter::ToString(GetFreeMemory()));
			Result.Body = JSONObject.ToString();
		}

		if (Query.GetURLPartsCount() == 2 && Query.CheckURLPart("version", 1)) {
			Result.Body = (LastVersion < 0x2000) ? "0" : Converter::ToHexString(LastVersion, 4);
			return;
		}

		if (Query.GetURLPartsCount() == 2 && Query.CheckURLPart("count", 1)) {
			Result.Body = Converter::ToString(MemoryStoredItems);
			return;
		}

		if (Query.GetURLPartsCount() == 2 && Query.CheckURLPart("freesize", 1)) {
			Result.Body = Converter::ToString(GetFreeMemory());
			return;
		}

		if (Query.GetURLPartsCount() == 2 && Query.CheckURLPart("types", 1)) {
			Result.Body = JSON::CreateStringFromIntVector<uint8_t>(GetItemsTypes(), 2);
			return;
		}

		if (Query.GetURLPartsCount() == 3 && Query.CheckURLPart("types", 1)) {
			map<string,string> Params = Query.GetParams();

			uint8_t		TypeID 	= Converter::UintFromHexString<uint8_t>(Query.GetStringURLPartByNumber(2));
			uint16_t	Items 	= CountItemsForTypeID(TypeID);

			uint16_t	Limit = 64;
			uint8_t		Page  = 0;

			if (Params.count("page") > 0)
				Page = Converter::UintFromHexString<uint8_t>(Params["page"]);

			if (Page >= ceil((float)Items / (float)Limit)- 1)
				Page = ceil((float)Items / (float)Limit) - 1;

			JSON JSONObject;
			JSONObject.SetItem("Limit", Converter::ToString(Limit));
			JSONObject.SetItem("Pages", Converter::ToString(ceil((float)Items / (float)Limit)));
			JSONObject.SetUintArray<uint16_t>("Items", GetItemsForTypeID(TypeID, Limit, Page), 3);

			Result.Body = JSONObject.ToString();
			return;
		}

		if (Query.GetURLPartsCount() == 2 && Query.CheckURLPart("history", 1)) {
			map<string,string> Params = Query.GetParams();

			uint16_t	Items = VersionHistoryCount();

			uint16_t	Limit = 256;
			uint8_t		Page  = 0;

			if (Params.count("page") > 0)
				Page = Converter::ToUint16(Params["page"]);

			if (Page >= ceil((float)Items / (float)Limit)- 1)
				Page = ceil((float)Items / (float)Limit) - 1;

			JSON JSONObject;
			JSONObject.SetItem("Limit", Converter::ToString(Limit));
			JSONObject.SetItem("Pages", Converter::ToString(ceil((float)Items / (float)Limit)));
			JSONObject.SetUintArray<uint16_t>("Items", VersionHistoryGet(Limit, Page), 4);

			Result.Body = JSONObject.ToString();

			return;
		}

		if (Query.GetURLPartsCount() == 3 && Query.CheckURLPart("history", 1) && Query.CheckURLPart("upgrade", 2)) {
			map<string,string> Params = Query.GetParams();

			uint16_t From 	= (Params.count("from") != 0) ? Converter::UintFromHexString<uint16_t>(Params["from"]) : 0x00;

			if (From != 0 && From < 0x2000) {
				Result.SetInvalid();
				return;
			}

			uint16_t To = 0x0;
			vector<Item> Items = GetUpgradeItems(From, To);

			string MQTTChunkHash = "";
			uint16_t ChunkPartID = 0;

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				httpd_resp_set_type	(Query.GetRequest(), HTTPD_TYPE_JSON);

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(),
					"{ \"From\": \"" + Converter::ToHexString(From,4) +"\", \"To\":\""
					+ Converter::ToHexString(To	,4) + "\", \"Items\":[");
			else {
				MQTTChunkHash = RemoteControl.StartChunk(Query.MQTTMessageID);
				RemoteControl.SendChunk("{ \"From\": \"" + Converter::ToHexString(From,4) +"\", \"To\":\""
						+ Converter::ToHexString(To	,4) + "\", \"Items\":[", MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);
			}

			vector<Item>::iterator it = Items.begin();
			// delete write items already in commit
			while (it != Items.end()) {
				JSON JSONObject;

				if (it->Header.TypeID == 0 && it->DataToString() == "") {
					Items.erase(Items.begin());
					continue;
				}

				JSONObject.SetItems(vector<pair<string,string>>({
		    		make_pair("ID"		, Converter::ToHexString(it->Header.MemoryID, 3)),
		    	    make_pair("TypeID"	, Converter::ToHexString(it->Header.TypeID  , 2)),
				    make_pair("Data"	, it->DataToString() )
				}));

				if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
					WebServer_t::SendChunk(Query.GetRequest(), JSONObject.ToString() + ((Items.size() > 1) ? "," : ""));
				else
					RemoteControl.SendChunk(JSONObject.ToString() + ((Items.size() > 1) ? "," : ""), MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);

				Items.erase(Items.begin());
			}

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
			{
				WebServer_t::SendChunk(Query.GetRequest(), "]}");
				WebServer_t::EndChunk(Query.GetRequest());
			}
			else {
				RemoteControl.SendChunk("]}", MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);
				RemoteControl.EndChunk(MQTTChunkHash, Query.MQTTMessageID);
			}

			Result.Body = "";

			if (Query.Transport == WebServer_t::QueryTransportType::MQTT)
				Result.ResponseCode = WebServer_t::Response::CODE::IGNORE;

			return;
		}

		if (Query.GetURLPartsCount() == 3 && Query.CheckURLPart("history", 1) && !Query.CheckURLPart("upgrade", 2)) {
			uint16_t VersionID = Converter::UintFromHexString<uint16_t>(Query.GetStringURLPartByNumber(2));
			Result.Body =  JSON::CreateStringFromIntVector<uint16_t>(GetItemsForVersion(VersionID), 4);

			return;
		}

		if (Query.GetURLPartsCount() == 2 || (Query.GetURLPartsCount() == 3 && Query.CheckURLPart("decode", 2))) {
			uint16_t ItemID = Converter::UintFromHexString<uint16_t>(Query.GetStringURLPartByNumber(1));

			if (ItemID > (DataSize / Settings.Storage.Data.ItemSize))
			{
				Result.SetInvalid();
				return;
			}

			Item Output = Read(ItemID);

			if ((Output.Header.Length == Settings.Memory.Empty8Bit && Output.Header.TypeID == Settings.Memory.Empty8Bit) ||
					ItemID != Output.Header.MemoryID) {
				Result.SetInvalid();
				return;
			}

			map<string,string> OutputMap = map<string,string>();

			if (Query.GetURLPartsCount() == 3) {
				Sensor_t *Sensor = Sensor_t::GetSensorByID(Output.Header.TypeID);

				if (Sensor != nullptr)
					OutputMap = Sensor->StorageDecode(Output.DataToString());
			}

			if (OutputMap.size() != 0)
				OutputMap["TypeID"] = Converter::ToHexString(Output.Header.TypeID,2);
			else {
				OutputMap["Data"] 	= Output.DataToString();
				OutputMap["Size"] 	= Converter::ToString(Output.Header.Length);
				OutputMap["TypeID"] = Converter::ToHexString(Output.Header.TypeID,2);
			}

			Output.ClearData();

			JSON JSONObject;
			for (auto& item : OutputMap)
				JSONObject.SetItem(item.first, item.second);

			OutputMap.clear();

			Result.Body = JSONObject.ToString();
		}
	}

	if (Query.Type == QueryType::POST) {
		if (Query.GetURLPartsCount() == 2 && Query.CheckURLPart("commit", 1)) {
			Commit();
			Result.SetSuccess();
			return;
		}

		if (Query.GetURLPartsCount() == 1) {
			map<string, string> 		Params 	= JSON(Query.GetBody()).GetItems();
			vector<map<string,string>> 	Items 	= vector<map<string,string>>();

			if (Params.empty()) {
				JSON JSONObject(Query.GetBody());
				if (JSONObject.GetType() == JSON::RootType::Array)
					Items = JSONObject.GetObjectsArray();
			}
			else
			{
				Items.push_back(map<string,string>());

				for (auto &MapItem : Params) {
					if (MapItem.first == "data") {
						const uint8_t PartLen = 100;
						while (Params[MapItem.first].size() > 0) {
							Items.at(0)[MapItem.first] += Params[MapItem.first].substr(0, (Params[MapItem.first].size() >= PartLen) ? PartLen : Params[MapItem.first].size());
							Params[MapItem.first] = (Params[MapItem.first].size() < PartLen) ? "" : Params[MapItem.first].substr(PartLen);
						}
					}
					else {
						Items.at(0)[MapItem.first] = MapItem.second;
						Params.erase(MapItem.first);
					}
				}
			}

			vector<uint16_t> InsertedIDs = vector<uint16_t>();

			vector<map<string,string>>::iterator it = Items.begin();
			while (it != Items.end()) {
				Item ItemToSave;

				if ((*it).count("id"))
					ItemToSave.Header.MemoryID	= Converter::UintFromHexString<uint16_t>((*it)["id"]);

				if ((*it).count("typeid"))
					ItemToSave.Header.TypeID	= Converter::UintFromHexString<uint8_t>((*it)["typeid"]);

				Sensor_t *Sensor = Sensor_t::GetSensorByID(ItemToSave.Header.TypeID);

				if (Sensor != nullptr) {
					ItemToSave.SetData(Sensor->StorageEncode((*it)));
					ItemToSave.Header.TypeID = Sensor->ID;
				}

				if (ItemToSave.GetData().size() == 0 && (*it).count("data"))
					ItemToSave.SetData((*it)["data"]);

				if (ItemToSave.GetData().size() == 0 || ItemToSave.Header.TypeID == Settings.Memory.Empty8Bit) {
					Result.SetInvalid();
					return;
				}

				InsertedIDs.push_back(Write(ItemToSave));

				it = Items.erase(it);
			}

			Result.ResponseCode = WebServer_t::Response::CODE::OK;

			if (InsertedIDs.size() == 1)
				Result.Body = "{\"success\" : \"true\", \"id\":\"" + Converter::ToHexString(InsertedIDs[0],3) + "\"}";
			else {
				Result.Body = "{\"success\" : \"true\", \"ids\": [";

				for (uint8_t i = 0; i<InsertedIDs.size(); i++) {
					Result.Body += "\"" + Converter::ToHexString(InsertedIDs[i],3) + "\"";
					if (i<InsertedIDs.size()-1) Result.Body += ",";
				}

				Result.Body += "]\"}";
			}
		}
	}

	if (Query.Type == QueryType::DELETE) {
		if (Query.GetURLPartsCount() == 1) {

			SPIFlash::EraseRange(VersionsStart, VersionsSize);
			SPIFlash::EraseRange(DataStart, DataSize);

			if (Patch->size() > 0)
				Patch->clear();

			AddressToWrite 			= VersionsStart;
			LastVersion 			= 0x1FFF;
			MemoryStoredItemsSize	= 0x0;
			MemoryStoredItems		= 0x0;

			Result.SetSuccess();
		}

		if (Query.GetURLPartsCount() == 2) {
			vector<string> IDsToDelete = Converter::StringToVector(Query.GetStringURLPartByNumber(1) , ",");

			for (auto& IDToDelete : IDsToDelete)
				Erase(Converter::UintFromHexString<uint16_t>(IDToDelete));

			if (IDsToDelete.size() > 0)
				Result.SetSuccess();
			else
			{
				Result.ResponseCode = WebServer_t::Response::CODE::INVALID;
				Result.Body = "{\"success\" : \"false\", \"message\":\"empty scenario ID\"}";
			}
		}
	}
}

string Storage_t::VersionToString() {
	return (LastVersion < 0x2000) ? "0" : Converter::ToHexString(LastVersion, 4);
}


uint16_t Storage_t::FindFreeID() {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	uint32_t VersionsStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;
	uint32_t VersionsSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.Size4MB : Settings.Storage.Versions.Size16MB;

	uint16_t Result = 0xFFF;

	// Use max avaliable bitset size
	bitset<Settings.Storage.Data.Size16MB/Settings.Storage.Data.ItemSize> *FreeIndexes = new bitset<Settings.Storage.Data.Size16MB/Settings.Storage.Data.ItemSize>(0);
	FreeIndexes->set();

	uint32_t Address = DataStart;
	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.MemoryID < FreeIndexes->size())
			FreeIndexes->set(Header.MemoryID, false);

		Address += Settings.Storage.Data.ItemSize;
	}

	for (int i=0;i< Patch->size(); i++)
		FreeIndexes->set(Patch->at(i).Header.MemoryID, (Patch->at(i).Header.Length == 0) ? true : false);

	for (uint16_t i=0; i < (DataSize/Settings.Storage.Data.ItemSize); i++)
		if (FreeIndexes->test(i))
		{
			Result = i;
			break;
		}

	delete FreeIndexes;
	return Result;
}

uint16_t Storage_t::WriteNow(Item Record) {
	uint32_t DataStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;


	uint32_t 			Address 		= DataStart;
	vector<uint32_t> 	AddressVector 	= vector<uint32_t>();
	uint8_t				tmpLength		= Record.Header.Length;

	MemoryStoredItemsSize += Record.Header.Length;
	MemoryStoredItems ++;

	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.TypeID == Settings.Memory.Empty8Bit && Header.MemoryID == Settings.Memory.Empty16Bit) {
			AddressVector.push_back(Address);
			tmpLength --;

			if (tmpLength == 0) break;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	if (Address > DataStart + DataSize - 1)
		return false;

	if (Record.GetData().size() == 0)
		return false;

	tmpLength = Record.Header.Length - 1;
	for (int i=0; i < AddressVector.size(); i++) {
		RecordHeader tmpHeader = Record.Header;
		tmpHeader.Length = tmpLength;
		SPIFlash::WriteUint32(tmpHeader.HeaderAsInteger, AddressVector[i]);
		tmpLength--;

		uint16_t PartSize = ((i+1) * RecordDataSize() < Record.GetData().size()) ? RecordDataSize() : Record.GetData().size() - i * RecordDataSize();

		RecordPartInfo PartInfo;
		PartInfo.CRC = Record.CRC16ForData(i * RecordDataSize(), PartSize);

		PartInfo.Size = PartSize - 1;

		SPIFlash::WriteUint32(PartInfo.PartInfoAsInteger, AddressVector[i] + 0x04);

		for (int j=0; j < RecordDataSize(); j+=4) {
			uint32_t Data = 0x0;
			for (int k=j; k < j + 4; k++) {
				Data = Data << 8;
				Data += (i * RecordDataSize() + k < Record.GetData().size()) ? Record.GetData()[i * RecordDataSize() + k] : 0xFF;
			}

			SPIFlash::WriteUint32(Data, AddressVector[i] + RecordHeaderSize() + j);

			if (i*RecordDataSize() + j + 4 >= Record.GetData().size())
				break;
		}
	}

	return ((AddressVector[0] - DataStart) / Settings.Storage.Data.ItemSize);
}

void IRAM_ATTR Storage_t::EraseNow(uint16_t ItemID) {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	uint32_t 			Address 		= DataStart;
	vector<uint32_t> 	AddressVector 	= vector<uint32_t>();

	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID == ItemID) {
			AddressVector.push_back(Address);
			if (Header.Length == 0x0) break;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	if (Address > (DataStart + DataSize -1 ))
		return;

	for (uint32_t AddressItem : AddressVector) {
		SPIFlash::EraseRange(AddressItem, Settings.Storage.Data.ItemSize);

		if (MemoryStoredItemsSize > 0) MemoryStoredItemsSize --;
	}

	MemoryStoredItems--;
}

uint32_t IRAM_ATTR Storage_t::GetFreeMemory() {
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	return ((DataSize / Settings.Storage.Data.ItemSize) - MemoryStoredItemsSize) * (Settings.Storage.Data.ItemSize - sizeof(RecordHeader) - sizeof(RecordPartInfo));
}

void IRAM_ATTR Storage_t::CalculateMemoryItemsData() {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	MemoryStoredItemsSize 	= 0x0;
	MemoryStoredItems		= 0x0;
	uint32_t Address		= DataStart;

	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit)
			MemoryStoredItemsSize++;

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.Length == 0)
			MemoryStoredItems++;

		Address += Settings.Storage.Data.ItemSize;
	}
}

vector<uint8_t> IRAM_ATTR Storage_t::GetItemsTypes() {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	vector<uint8_t> Result 	= vector<uint8_t>();
	uint32_t 		Address = DataStart;

	while (Address < DataStart + DataSize) {
		RecordHeader Header;

		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.TypeID != Settings.Memory.Empty8Bit)
			if (find(Result.begin(), Result.end(), Header.TypeID) == Result.end()) {
				Result.push_back(Header.TypeID);
			}

		Address += Settings.Storage.Data.ItemSize;
	}

	return Result;
}
uint16_t IRAM_ATTR Storage_t::CountItemsForTypeID(uint8_t TypeID) {
	uint16_t Result = 0;

	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	uint32_t Address = DataStart;

	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.TypeID == TypeID && Header.Length == 0)
			Result ++;

		Address += Settings.Storage.Data.ItemSize;
	}

	return Result;
}

vector<uint16_t> Storage_t::GetItemsForTypeID(uint8_t Type, uint16_t Limit, uint8_t Page) {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	vector<uint16_t> Result = vector<uint16_t>();

	uint16_t Counter = 0;

	uint32_t Address = DataStart;
	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.TypeID == Type && Header.Length == 0) {
			if ((Counter >= Limit*Page) && (Counter < Limit* (Page+1)))
				Result.push_back(Header.MemoryID);

			Counter++;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	return Result;
}

bool IRAM_ATTR Storage_t::IsExist(uint16_t ID) {
	uint32_t DataStart 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.StartAddress4MB : Settings.Storage.Data.StartAddress16MB;
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;

	uint32_t Address = DataStart;

	while (Address < DataStart + DataSize) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID == ID)
			return true;

		Address += Settings.Storage.Data.ItemSize;
	}

	return false;
}

uint16_t IRAM_ATTR Storage_t::VersionHistoryCount() {
	uint32_t VersionsStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;
	uint32_t VersionsSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.Size4MB : Settings.Storage.Versions.Size16MB;

	uint16_t Count = 0;

	uint32_t Address = VersionsStart;
	while (Address < VersionsStart + VersionsSize) {
		uint32_t DataPart = SPIFlash::ReadUint32(Address);

		uint16_t LeftPart	= (uint16_t)((DataPart << 16) >> 16);
		uint16_t RightPart	= (uint16_t)(DataPart >> 16);

		if (LeftPart >= 0x2000  && LeftPart !=Settings.Memory.Empty16Bit)	Count++;
		if (RightPart >= 0x2000 && RightPart!=Settings.Memory.Empty16Bit)	Count++;

		if (LeftPart == Settings.Memory.Empty16Bit || RightPart == Settings.Memory.Empty16Bit)
			break;

		Address += 0x4;
	}

	return Count;
}

vector<uint16_t> IRAM_ATTR Storage_t::VersionHistoryGet(uint16_t Limit, uint8_t Page) {
	uint32_t VersionsStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;
	uint32_t VersionsSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.Size4MB : Settings.Storage.Versions.Size16MB;

	vector<uint16_t> Result = vector<uint16_t>();

	uint16_t Counter = 0;

	uint32_t Address = VersionsStart;
	while (Address < VersionsStart + VersionsSize) {
		uint32_t DataPart = SPIFlash::ReadUint32(Address);

		uint16_t LeftPart	= (uint16_t)((DataPart << 16) >> 16);
		uint16_t RightPart	= (uint16_t)(DataPart >> 16);

		if ((LeftPart >= 0x2000 && LeftPart != Settings.Memory.Empty16Bit) &&
				(Counter >= Limit*Page) && (Counter < Limit* (Page+1))) {
			Result.push_back(LeftPart);
			Counter++;
		}

		if ((RightPart >= 0x2000 && RightPart != Settings.Memory.Empty16Bit) &&
				(Counter >= Limit*Page) && (Counter < Limit* (Page+1))) {
			Result.push_back(RightPart);
			Counter++;
		}

		if (LeftPart == Settings.Memory.Empty16Bit || RightPart == Settings.Memory.Empty16Bit)
			break;

		Address += 0x4;
	}

	return Result;
}

vector<uint16_t> IRAM_ATTR Storage_t::GetItemsForVersion(uint16_t Version) {
	uint32_t LastAddress;
	return GetItemsForVersion(Version, LastAddress);
}

vector<uint16_t> IRAM_ATTR Storage_t::GetItemsForVersion(uint16_t Version, uint32_t &LastAddress, uint32_t StartAdress) {
	uint32_t VersionsStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;
	uint32_t VersionsSize 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.Size4MB : Settings.Storage.Versions.Size16MB;

	vector<uint16_t> Result = vector<uint16_t>();
	uint32_t LastAddressIn = LastAddress;

	if (StartAdress == UINT32MAX)
		StartAdress = (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;

	bool VersionFinded = false;

	uint32_t Address = StartAdress;
	while (Address < VersionsStart + VersionsSize) {
		uint32_t DataPart = SPIFlash::ReadUint32(Address);

		uint16_t LeftPart	= (uint16_t)((DataPart << 16) >> 16);
		uint16_t RightPart	= (uint16_t)(DataPart >> 16);

		if (LeftPart == Version) {
			VersionFinded = true;

			if (RightPart < 0x2000)
				Result.push_back(RightPart);
			else
				break;

			goto increment;
		}
		else if (RightPart == Version) {
			VersionFinded = true;
			goto increment;
		}
		else if (!VersionFinded && (LeftPart > Version || RightPart > Version))
		{
			break;
		}
		else if (VersionFinded && LeftPart > 0x2000) {
			break;
		}
		else if (VersionFinded && RightPart > 0x2000) {
			Result.push_back(LeftPart);
			break;
		}
		else if (VersionFinded) {
			Result.push_back(LeftPart);
			Result.push_back(RightPart);
		}

		increment:
		LastAddress = Address;
		Address += 0x4;
	}

	return Result;
}

vector<Storage_t::Item> IRAM_ATTR Storage_t::GetUpgradeItems(uint16_t From, uint16_t &ToCurrent) {
	uint32_t DataSize 		= (Settings.DeviceGeneration < 2) ? Settings.Storage.Data.Size4MB : Settings.Storage.Data.Size16MB;
	uint32_t VersionsStart 	= (Settings.DeviceGeneration < 2) ? Settings.Storage.Versions.StartAddress4MB : Settings.Storage.Versions.StartAddress16MB;

	vector<Item> Result 	= vector<Item>();
	vector<Item> ResultPart	= vector<Item>();
	uint32_t LastAddress 	= VersionsStart;

	if (From == 0)
		From = 0x1FFF;

	if (From == LastVersion)
	{
		ToCurrent = (LastVersion >= 0x2000) ? LastVersion : 0;
		return vector<Storage_t::Item>();
	}

	uint8_t		TotalCount 	= 0;
	uint8_t		PartCount 	= 0;

	uint16_t	SavedTo 	= ToCurrent;

	while (TotalCount + PartCount < Settings.Storage.Versions.VersionMaxSize && From <= LastVersion)
	{
		From++;
		PartCount = 0;

		for (auto& FindedItem: GetItemsForVersion(From, LastAddress, LastAddress)) {
			if ((uint16_t)((FindedItem << 4) >> 4) <= (DataSize / Settings.Storage.Data.ItemSize)) {
				if (FindedItem < 0x1000) {
					Item ReadedItem = Read(FindedItem);

					if (ReadedItem.Header.MemoryID != Settings.Memory.Empty16Bit) {
						ResultPart.push_back(ReadedItem);

						PartCount += ReadedItem.Header.Length;
					}
				}

				if (FindedItem >= 0x1000 && FindedItem < 0x2000) {
					Item ItemToDelete;
					ItemToDelete.Header.MemoryID = FindedItem - 0x1000;
					ResultPart.push_back(ItemToDelete);
					PartCount++;
				}
			}
		}

		if (PartCount + TotalCount > Settings.Storage.Versions.VersionMaxSize)
			break;
		else if (PartCount == 0 && From < LastVersion)
		{
			continue;
		}
		else
		{
			Result.insert(Result.end(), ResultPart.begin(), ResultPart.end());
			ResultPart.clear();
			TotalCount += PartCount;

			ToCurrent = From;

			if (From < LastVersion)
				ToCurrent++;
		}
	}

	if (ToCurrent > 0x2000)
		ToCurrent--;

	// clean up history to prevent dublicates
    vector<Item>::iterator it = Result.end();
    vector<uint16_t> Indexes = {};
    while (it > Result.begin()) {
        it--;
    	uint16_t ID = it->Header.MemoryID;

    	if (find(Indexes.begin(), Indexes.end(), ID) != Indexes.end())
    		it = Result.erase(it);
    	else
    		Indexes.push_back(ID);
	}

	return Result;
}

uint8_t Storage_t::RecordHeaderSize() {
	return sizeof(RecordHeader) + sizeof(RecordPartInfo);
}

uint16_t Storage_t::RecordDataSize() {
	return Settings.Storage.Data.ItemSize - RecordHeaderSize();
}
