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

using namespace std;

class DataEndpoint_t {
	public:
		static const char		*Tag;
		static string 			NVSArea;

		static	DataEndpoint_t*	GetForDevice();
		virtual void 			Init() {};

		virtual void 			HandleHTTPRequest(
				WebServer_t::Response &,
				QueryType, vector<string>,
				map<string,string>,
				string RequestBody,
				httpd_req_t *Request = NULL,
				WebServer_t::QueryTransportType TransportType = WebServer_t::QueryTransportType::WebServer,
				int MsgID = 0) {			ESP_LOGE("!!!!","???");};
};

#include "../data_endpoints/DataRemote.cpp"

#endif