/*
*    Query.h
*    Class designed to parse HTTP query and make suitable struct to work with it's data
*
*/

#ifndef QUERY_H
#define QUERY_H

using namespace std;

#include <string>
#include <vector>
#include <map>

#include "WebServer.h"

#include "HTTPClient.h"

class Query_t {
	public:
		QueryType           Type = QueryType::NONE;
		WebServer_t::QueryTransportType
							Transport = WebServer_t::QueryTransportType::WebServer;

		int					MQTTMessageID = 0;

		Query_t(const char  *Data	, WebServer_t::QueryTransportType Transport = WebServer_t::QueryTransportType::MQTT);
		Query_t(httpd_req_t *Request, QueryType Type);

		bool 				CheckURLMask(string URLMask);
		bool				CheckURLPart(string Needle, uint8_t Number);
		uint8_t				GetURLPartsCount();
		string				GetStringURLPartByNumber(uint8_t Number);
		const char*			GetLastURLPartPointer();


		const char*			GetBody();
		map<string,string>  GetParams();
		httpd_req_t* 		GetRequest();

		void 				Cleanup();
	private:
		void				ProcessData();
		void				Dump();

		httpd_req_t 		*Request		= NULL;
		const char*			URL				= NULL;

		const char*			BodyPointer 	= NULL;
		char*				IsolatedBody	= NULL;

		vector<pair<uint16_t,uint16_t>>
							PartsData = vector<pair<uint16_t,uint16_t>>();
};

#endif
