/*
*    Network.cpp
*    Class to handle API /Device
*
*/
#include "Globals.h"
#include "Data.h"

const char *DataEndpoint_t::Tag 	= "Data_t";
string		DataEndpoint_t::NVSArea	= "Data";

DataEndpoint_t* DataEndpoint_t::GetForDevice() {
	switch (Settings.eFuse.Type)
	{
		case 0x81	: return new DataRemote_t();
		default		: return new DataEndpoint_t();
	}

}
