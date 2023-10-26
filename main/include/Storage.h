/*
*    Storage.h
*    Class for handling storage functions
*
*/
#ifndef STORAGE_H
#define STORAGE_H

using namespace std;

#include <string>
#include <map>
#include <bitset>

#include "Query.h"

#include "Memory.h"
#include "WebServer.h"
#include "JSON.h"
#include "DateTime.h"

class Storage_t {
	public:
		typedef union __attribute__((__packed__)) {
			struct { uint32_t TypeID : 8, MemoryID : 16, Length : 8; };
			uint32_t HeaderAsInteger = 0xFFFFFF00;
		} RecordHeader;

		typedef union __attribute__((__packed__)) {
			struct { uint32_t CRC : 16, Size : 8, Reserved : 8; };
			uint32_t PartInfoAsInteger = 0x00000000;
		} RecordPartInfo;

		struct Item {
			public:
				RecordHeader 			Header;

				void 					SetData(string);
				void 					SetData(uint8_t);
				void 					SetData(vector<uint8_t>);
				void					ClearData();

				vector<uint8_t>			GetData();
				string					DataToString();
				uint16_t				CRC16ForData(uint16_t Start, uint16_t Length);
			private:
				vector<uint8_t>			Data = vector<uint8_t>();
				void					CalculateLength();
		};

		void Init();

		Item 							Read	(uint16_t ItemID);
		//vector<Item> 					Read	(vector<uint16_t> ItemID);
		uint16_t 						Write	(Item Record);
		void 							Erase	(uint16_t ItemID);
		void 							Commit	(uint16_t Version = 0);

		uint16_t						CurrentVersion();

	    void							HandleHTTPRequest(WebServer_t::Response &, Query_t &Query);
	    string							VersionToString();

	private:

		vector<Item>					*Patch = new vector<Item>();
		uint32_t						AddressToWrite 			= 0x0;

		uint16_t						LastVersion 			= 0x1FFF;
		uint16_t						MemoryStoredItemsSize	= 0x0;
		uint16_t						MemoryStoredItems		= 0x0;

		uint16_t 						FindFreeID();
		uint16_t 						WriteNow	(Item Record);
		void							EraseNow	(uint16_t ItemID);

		uint32_t						GetFreeMemory();

		void							CalculateMemoryItemsData();

		vector<uint8_t>					GetItemsTypes();
		uint16_t						CountItems();
		uint16_t						CountItemsForTypeID(uint8_t TypeID);
		vector<uint16_t>				GetItemsForTypeID(uint8_t TypeID, uint16_t Limit, uint8_t Page);
		bool							IsExist(uint16_t ID);

		uint16_t						VersionHistoryCount();
		vector<uint16_t>				VersionHistoryGet(uint16_t Limit, uint8_t Page);
		vector<uint16_t>				GetItemsForVersion(uint16_t Version);
		vector<uint16_t>				GetItemsForVersion(uint16_t Version, uint32_t &LastAddress, uint32_t StartAdress = UINT32MAX);
		vector<Storage_t::Item>			GetUpgradeItems(uint16_t From, uint16_t &ToCurrent);

		static uint8_t					RecordHeaderSize();
		static uint16_t					RecordDataSize();
};

#endif
