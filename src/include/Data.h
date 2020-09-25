/*
*    Data.h
*    Class to handle API /Data
*
*/

#ifndef DATA_H
#define DATA_H

#include <esp_netif_ip_addr.h>

#include "WebServer.h"
#include "WiFi.h"
#include "Memory.h"

#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
#include <hap.h>
#include "hap_apple_chars.h"
#include "hap_apple_servs.h"
#include <HomeKit.h>

#endif

using namespace std;

class DataEndpoint_t {
	public:
		static const char		*Tag;
		static string 			NVSArea;

		static	DataEndpoint_t*	GetForDevice();
		virtual void 			Init() {};

		virtual void 			HandleHTTPRequest(WebServer_t::Response &, Query_t &) { };
};

#include "../data_endpoints/DataRemote.cpp"

#endif
