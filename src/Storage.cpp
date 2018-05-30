/*
*    Storage.cpp
*    Class for handling storage functions
*
*/

#include "Storage.h"
#include "Globals.h"

static char tag[] = "Storage";

Storage_t::Storage_t() {
	AddressToWrite = Settings.Storage.Versions.StartAddress;
	while (AddressToWrite < Settings.Storage.Versions.StartAddress + Settings.Storage.Versions.Size) {
		uint16_t FindedID = SPIFlash::ReadUint16(AddressToWrite);

		if (FindedID == Settings.Memory.Empty16Bit)
			break;

		if (FindedID > LastVersion)
			LastVersion = FindedID;

		AddressToWrite += 2;
	}
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
	uint32_t 			Address 		= Settings.Storage.Data.StartAddress;
	vector<uint32_t> 	AddressVector 	= vector<uint32_t>();
	Storage_t::Item 	Result;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger 	= SPIFlash::ReadUint32(Address);

		if (Header.MemoryID == ItemID) {
			AddressVector.push_back(Address);
			if (Header.Length == 0x0) break;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	if (Address > Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size)
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
	if (Record.Header.MemoryID== 0xFFFF)
		Record.Header.MemoryID = FindFreeID();

	if (Record.Header.MemoryID == 0xFFFF)
		return 0xFFFF;

	if (Record.Header.MemoryID >= Settings.Storage.Data.Size / Settings.Storage.Data.ItemSize - 1)
		Record.Header.MemoryID = Settings.Storage.Data.Size / Settings.Storage.Data.ItemSize - 1;

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

		map<uint16_t,uint8_t> Items = {};

		while (Patch->front().Header.Length + Size < Settings.Storage.Versions.VersionMaxSize && Patch->size() > 0) {
			Size += (Patch->front().Header.Length > 0) ? Patch->front().Header.Length : 1 ;

			if (Patch->front().Header.Length == 0) {
				EraseNow(Patch->front().Header.MemoryID);
				Items[Patch->front().Header.MemoryID + 0x1000] = Patch->front().Header.TypeID;
			}
			else {
				WriteNow(Patch->front());
				Items[Patch->front().Header.MemoryID] = Patch->front().Header.TypeID;
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


void Storage_t::HandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody) {
	if (Type == QueryType::GET) {
		if (URLParts.size() == 0) {
			JSON JSONObject;

			JSONObject.SetItem("Version" , (LastVersion < 0x2000) ? "0" : Converter::ToHexString(LastVersion, 4));
			JSONObject.SetItem("Count"	 , Converter::ToString(CountItems()));
			JSONObject.SetItem("Freesize", Converter::ToString(GetFreeMemory()));

			Result.Body = JSONObject.ToString();
		}

		if (URLParts.size() == 1 && URLParts[0] == "types") {
			Result.Body = JSON::CreateStringFromIntVector<uint8_t>(GetItemsTypes(), 2);
			return;
		}

		if (URLParts.size() == 2 && URLParts[0] == "types") {
			uint8_t		TypeID 	= Converter::UintFromHexString<uint8_t>(URLParts[1]);
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

		if (URLParts.size() == 1 && URLParts[0] == "history") {
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

		if (URLParts.size() == 2 && URLParts[0] == "history" && URLParts[1] == "upgrade") {
			uint16_t From 	= (Params.count("from") != 0) ? Converter::UintFromHexString<uint16_t>(Params["from"]) : 0x00;

			if (From != 0 && From < 0x2000) {
				Result.SetInvalid();
				return;
			}

			uint16_t To = 0x0;
			vector<Item> Items = GetUpgradeItems(From, To);

			JSON JSONObject;

			JSONObject.SetItems(vector<pair<string,string>>({
	    		make_pair("From", Converter::ToHexString(From,4)),
	    	    make_pair("To"	, Converter::ToHexString(To	,4))
			}));

			vector<vector<pair<string,string>>> Output = vector<vector<pair<string,string>>>();

			vector<Item>::iterator it = Items.begin();
			// delete write items already in commit
			while (it != Items.end()) {
				Output.push_back(vector<pair<string,string>>({
		    		make_pair("ID"		, Converter::ToHexString(it->Header.MemoryID, 3) ),
				    make_pair("TypeID"	, Converter::ToHexString(it->Header.TypeID  , 2) ),
				    make_pair("Data"	, it->DataToString() )
					}));

				Items.erase(Items.begin());
			}

			JSONObject.SetObjectsArray("Items", Output);

			Result.Body = JSONObject.ToString();

			return;
		}

		if (URLParts.size() == 2 && URLParts[0] == "history") {
			uint16_t VersionID = Converter::UintFromHexString<uint16_t>(URLParts[1]);
			Result.Body =  JSON::CreateStringFromIntVector<uint16_t>(GetItemsForVersion(VersionID), 4);

			return;
		}

		if (URLParts.size() == 1 || (URLParts.size() == 2 && URLParts[1] == "decode")) {
			uint16_t ItemID = Converter::UintFromHexString<uint16_t>(URLParts[0]);

			if (ItemID > (Settings.Storage.Data.Size / Settings.Storage.Data.ItemSize))
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

			if (URLParts.size() == 2) {
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

	if (Type == QueryType::POST) {
		if (URLParts.size() == 1 && URLParts[0] == "commit") {
			Commit();
			Result.SetSuccess();
			return;
		}

		if (URLParts.size()  == 0) {
			vector<map<string,string>> Items = vector<map<string,string>>();

			if (Params.empty()) {
				JSON JSONObject(RequestBody);
				if (JSONObject.GetType() == JSON::RootType::Array)
					Items = JSONObject.GetObjectsArray();

				RequestBody = "";
			}
			else {
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

	if (Type == QueryType::DELETE) {
		if (URLParts.size() == 1) {
			vector<string> IDsToDelete = Converter::StringToVector(URLParts[0], ",");

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

uint16_t Storage_t::FindFreeID() {
	uint16_t Result = 0xFFF;

	bitset<Settings.Storage.Data.Size/Settings.Storage.Data.ItemSize> *FreeIndexes = new bitset<Settings.Storage.Data.Size/Settings.Storage.Data.ItemSize>(0);
	FreeIndexes->set();

	uint32_t Address = Settings.Storage.Data.StartAddress;
	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.MemoryID < FreeIndexes->size())
			FreeIndexes->set(Header.MemoryID, false);

		Address += Settings.Storage.Data.ItemSize;
	}

	for (int i=0;i< Patch->size(); i++)
		FreeIndexes->set(Patch->at(i).Header.MemoryID, (Patch->at(i).Header.Length == 0) ? true : false);

	for (uint16_t i=0; i < FreeIndexes->size(); i++)
		if (FreeIndexes->test(i)) {
			Result = i;
			break;
		}

	delete FreeIndexes;
	return Result;
}

uint16_t Storage_t::WriteNow(Item Record) {
	uint32_t 			Address 		= Settings.Storage.Data.StartAddress;
	vector<uint32_t> 	AddressVector 	= vector<uint32_t>();
	uint8_t				tmpLength		= Record.Header.Length;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.TypeID == Settings.Memory.Empty8Bit && Header.MemoryID == Settings.Memory.Empty16Bit) {
			AddressVector.push_back(Address);
			tmpLength --;

			if (tmpLength == 0) break;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	if (Address > Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size - 1)
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

	return ((AddressVector[0] - Settings.Storage.Data.StartAddress) / Settings.Storage.Data.ItemSize);
}

void Storage_t::EraseNow(uint16_t ItemID) {
	uint32_t 			Address 		= Settings.Storage.Data.StartAddress;
	vector<uint32_t> 	AddressVector 	= vector<uint32_t>();

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID == ItemID) {
			AddressVector.push_back(Address);
			if (Header.Length == 0x0) break;
		}

		Address += Settings.Storage.Data.ItemSize;
	}

	if (Address > (Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size -1 ))
		return;

	for (uint32_t AddressItem : AddressVector)
		SPIFlash::EraseRange(AddressItem, Settings.Storage.Data.ItemSize);
}

uint32_t Storage_t::GetFreeMemory() {
	return ((Settings.Storage.Data.Size / Settings.Storage.Data.ItemSize) - GetMemoryItemsLen()) * (Settings.Storage.Data.ItemSize - sizeof(RecordHeader) - sizeof(RecordPartInfo));
}

uint16_t Storage_t::GetMemoryItemsLen() {
	uint16_t MemoryContentLen 	= 0x0;
	uint32_t Address			= Settings.Storage.Data.StartAddress;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit)
			MemoryContentLen++;

		Address += Settings.Storage.Data.ItemSize;
	}

	return MemoryContentLen;
}

vector<uint8_t> Storage_t::GetItemsTypes() {
	vector<uint8_t> Result = vector<uint8_t>();

	uint32_t Address = Settings.Storage.Data.StartAddress;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.TypeID != Settings.Memory.Empty8Bit)
			if(find(Result.begin(), Result.end(), Header.TypeID) == Result.end())
				Result.push_back(Header.TypeID);

		Address += Settings.Storage.Data.ItemSize;
	}

	return Result;
}
uint16_t Storage_t::CountItemsForTypeID(uint8_t TypeID) {
	uint16_t Result = 0;

	uint32_t Address = Settings.Storage.Data.StartAddress;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.TypeID == TypeID && Header.Length == 0)
			Result ++;

		Address += Settings.Storage.Data.ItemSize;
	}

	return Result;
}

uint16_t Storage_t::CountItems() {
	uint16_t Result = 0;

	uint32_t Address = Settings.Storage.Data.StartAddress;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID != Settings.Memory.Empty16Bit && Header.Length == 0)
			Result ++;

		Address += Settings.Storage.Data.ItemSize;
	}

	return Result;
}

vector<uint16_t> Storage_t::GetItemsForTypeID(uint8_t Type, uint16_t Limit, uint8_t Page) {
	vector<uint16_t> Result = vector<uint16_t>();

	uint16_t Counter = 0;

	uint32_t Address = Settings.Storage.Data.StartAddress;
	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
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

bool Storage_t::IsExist(uint16_t ID) {
	uint32_t Address = Settings.Storage.Data.StartAddress;

	while (Address < Settings.Storage.Data.StartAddress + Settings.Storage.Data.Size) {
		RecordHeader Header;
		Header.HeaderAsInteger = SPIFlash::ReadUint32(Address);

		if (Header.MemoryID == ID)
			return true;

		Address += Settings.Storage.Data.ItemSize;
	}

	return false;
}

uint16_t Storage_t::VersionHistoryCount() {
	uint16_t Count = 0;

	uint32_t Address = Settings.Storage.Versions.StartAddress;
	while (Address < Settings.Storage.Versions.StartAddress + Settings.Storage.Versions.Size) {
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

vector<uint16_t> Storage_t::VersionHistoryGet(uint16_t Limit, uint8_t Page) {
	vector<uint16_t> Result = vector<uint16_t>();

	uint16_t Counter = 0;

	uint32_t Address = Settings.Storage.Versions.StartAddress;
	while (Address < Settings.Storage.Versions.StartAddress + Settings.Storage.Versions.Size) {
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

vector<uint16_t> Storage_t::GetItemsForVersion(uint16_t Version) {
	uint32_t LastAddress;
	return GetItemsForVersion(Version, LastAddress);
}

vector<uint16_t> Storage_t::GetItemsForVersion(uint16_t Version, uint32_t &LastAddress, uint32_t StartAdress) {
	vector<uint16_t> Result = vector<uint16_t>();

	bool VersionFinded = false;

	uint32_t Address = StartAdress;
	while (Address < Settings.Storage.Versions.StartAddress + Settings.Storage.Versions.Size) {
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

vector<Storage_t::Item> Storage_t::GetUpgradeItems(uint16_t From, uint16_t &ToCurrent) {
	vector<Item> Result 	= vector<Item>();
	vector<Item> ResultPart	= vector<Item>();
	uint32_t LastAddress 	= Settings.Storage.Versions.StartAddress;

	if (From == 0)
		From = 0x1FFF;

	uint8_t		TotalCount 	= 0;
	uint8_t		PartCount 	= 0;

	while (TotalCount + PartCount < Settings.Storage.Versions.VersionMaxSize && From <= LastVersion) {
		From++;
		PartCount = 0;

		for (auto& FindedItem: GetItemsForVersion(From, LastAddress, LastAddress)) {
			if (FindedItem < 0x1000) {
				ResultPart.push_back(Read(FindedItem));
				PartCount += ResultPart.back().Header.Length;
			}

			if (FindedItem >= 0x1000 && FindedItem < 0x2000) {
				Item ItemToDelete;
				ItemToDelete.Header.MemoryID = FindedItem - 0x1000;
				ResultPart.push_back(ItemToDelete);
				PartCount++;
			}
		}

		if (PartCount + TotalCount > Settings.Storage.Versions.VersionMaxSize)
			break;
		else {
			Result.insert(Result.end(), ResultPart.begin(), ResultPart.end());
			ResultPart.clear();
			TotalCount += PartCount;

			ToCurrent = From;
		}
	}

	return Result;
}

uint8_t Storage_t::RecordHeaderSize() {
	return sizeof(RecordHeader) + sizeof(RecordPartInfo);
}

uint16_t Storage_t::RecordDataSize() {
	return Settings.Storage.Data.ItemSize - RecordHeaderSize();
}
