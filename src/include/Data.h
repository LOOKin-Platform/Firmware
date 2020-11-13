/*
*    Data.h
*    Class to handle API /Data
*
*    For second gen devices there are maximum 1024 records can be saved and minimum size for 1 record is 2048 byte
*
*/

#ifndef DATA_H
#define DATA_H

#include <esp_netif_ip_addr.h>

#include "WebServer.h"
#include "WiFi.h"
#include "Memory.h"

#include <hap.h>
#include "hap_apple_chars.h"
#include "hap_apple_servs.h"

using namespace std;

#define FREE_MEMORY_NVS		"FreeMemoryAddr"

class DataEndpoint_t {
	public:
		static 	const char		*Tag;
		static 	string 			NVSArea;
		const 	char*			PartitionName 	= "dataitems";

		static 	const uint16_t	MinRecordSize 	= 2048;
		static 	const uint16_t	MaxRecordsNum 	= 1024;
		static 	const uint16_t	MemoryBlockSize = 4096;

		static	DataEndpoint_t*	GetForDevice();
		virtual void 			Init() {};

		virtual void 			HandleHTTPRequest(WebServer_t::Response &, Query_t &) { };
		virtual string			RootInfo() { return "{}"; }

		void					Defragment();

	private:
		pair<uint32_t, uint32_t>SplitAddres(uint64_t FullAddress);
		uint64_t				JoinAddress(uint32_t Address, uint32_t Size);
		uint32_t 				NormalizedItemAddress(uint32_t Address);
		uint32_t				GetFreeMemoryAddress();
		void					SetFreeMemoryAddress(uint32_t);

		//void					Defragment();
		void					Move(uint32_t NewAddress, uint32_t OldAddress, uint32_t Size);
		void 					EraseRange(uint32_t Start, uint32_t Length);
	protected:
		bool					IsHomeKitEnabled();
		bool 					IsHomeKitExperimental();

		bool					SaveItem(string ItemName, string Item);
		string					GetItem(string ItemName);
		bool					DeleteItem(string ItemName);
		bool					DeleteStartedWith(string Key);
		void					EraseAll();
		void					Debug(string Tag="");
};

#include "../data_endpoints/DataRemote.cpp"

#endif
