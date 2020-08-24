/*
*    Query.cpp
*    Class designed to parse HTTP query and make suitable struct to work with it's data
*
*/

using namespace std;

#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <string>

#include "JSON.h"
#include "Query.h"
#include "Converter.h"

#include <esp_log.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

Query_t::Query_t(const char *Data, WebServer_t::QueryTransportType sTransport)
{
	Transport 	= sTransport;
	URL 		= Data;

	ProcessData();

	if (URL != NULL && strlen(URL) > 6)
	{
		if (strstr(URL,"GET") == URL)
			Type = QueryType::GET;
		else if (strstr(URL, "POST") == URL)
			Type = QueryType::POST;
		else if (strstr(URL, "PUT") == URL)
			Type = QueryType::PUT;
		else if (strstr(URL, "PATCH") == URL)
			Type = QueryType::PATCH;
		else if (strstr(URL, "DELETE") == URL)
			Type = QueryType::DELETE;
	}

	//Dump();
}

Query_t::Query_t(httpd_req_t *sRequest, QueryType sType)
{
	Type 	= sType;
	Request = sRequest;

	if (Request != NULL)
		URL = Request->uri;

	ProcessData();

	//Dump();
}

void Query_t::Cleanup() {

	// As all Body data used in JSON after it was received JSON lib free all alocated data
	// So it no need to cleanup IsolatedBody
	//return;

	if (IsolatedBody != NULL)
		delete [] IsolatedBody;
}

void Query_t::ProcessData() {
	if (URL == NULL)
		return;

	PartsData.clear();

	if (Transport == WebServer_t::QueryTransportType::WebServer && Request != NULL) {
	    if (Request->content_len > 0) {
	    	IsolatedBody = new char[Request->content_len + 1];

	    	char Buffer[512];

	    	size_t ReadedContentLen = 0;
	    	while (ReadedContentLen < Request->content_len) {
	    		int ret = ::httpd_req_recv(Request, Buffer, MIN (Request->content_len - ReadedContentLen, sizeof(Buffer)));

	    		if (ret > 0) {
		    	    ::memcpy(&IsolatedBody[ReadedContentLen], Buffer, ret);
	    			ReadedContentLen += ret;
		    	    IsolatedBody[ReadedContentLen] = '\0';
	    		}
	    		else
	    		{
	    			ESP_LOGE("Query", "ret error");

	    			if (ret == HTTPD_SOCK_ERR_TIMEOUT)
	    				httpd_resp_send_408(Request);

	    			delete[] IsolatedBody;

	    			break;
	    		}
	    	}
	    }
	}

	const char *Next 			= URL;
	const char *ParamsPtr 		= ::strstr(URL, "?");

	uint8_t NewLineLength		= 2;
	const char *BodyPtr			= ::strstr(URL, "\r\n");

	if (BodyPtr == NULL)
	{
		BodyPtr					= ::strstr(URL, "\n");
		NewLineLength 			= 1;
	}

	uint16_t LastOffset 		= 0;
	uint16_t ParamsOffset		= (ParamsPtr == NULL) 	? 0 : (uint16_t)(ParamsPtr 	- URL);
	uint16_t BodyOffset			= (BodyPtr == NULL) 	? 0 : (uint16_t)(BodyPtr 	- URL);

	while ((Next = strstr(Next, "/")) != NULL)
	{
		uint16_t CurrentOffset = (uint16_t)(Next - URL);
		if ( CurrentOffset > ParamsOffset && ParamsOffset != 0) {
			Next = ParamsPtr;
			break;
		} else if ( CurrentOffset > BodyOffset && BodyOffset != 0) {
			Next = BodyPtr;
			break;
		}

		if (Next != URL) {
			//ESP_LOGE("FIRST", "%d", LastOffset + 1);
			//ESP_LOGE("SECOND", "%d", (uint16_t)(Next - URL) - LastOffset - 1);
			PartsData.push_back(pair<uint16_t, uint16_t>(LastOffset + 1, (uint16_t)(Next - URL) - LastOffset - 1));
		}

		LastOffset = (uint16_t)(Next - URL);

	    ++Next;
	}

	if (LastOffset <= strlen(URL)) {
		uint16_t ParamLength = strlen(URL) - LastOffset - 1;
		if (BodyOffset 		> 0) 	ParamLength = BodyOffset 	- LastOffset - 1;
		if (ParamsOffset 	> 0)	ParamLength = ParamsOffset 	- LastOffset - 1;

		//ESP_LOGE("LastOffset + 1"	, "%d", LastOffset + 1);
		//ESP_LOGE("Offset"			, "%d", ParamLength);

		PartsData.push_back(pair<uint16_t, uint16_t>(LastOffset + 1, ParamLength));
	}

	if (PartsData.size() > 0 && PartsData.back().second == 0)
		PartsData.pop_back();

	if (PartsData.size() > 0 && Type == QueryType::NONE)
		PartsData.erase(PartsData.begin(),PartsData.begin()+1);

	if (BodyOffset > 0)
		BodyPointer = BodyPtr + NewLineLength;
}


httpd_req_t* Query_t::GetRequest()
{
	return Request;
}

bool Query_t::CheckURLPart(string Needle, uint8_t Number) {
	if (Number >= PartsData.size()) return false;

	if (PartsData[Number].first + Needle.size() > strlen(URL))
		return false;

	string Haystack(URL + PartsData[Number].first, Needle.size());

	if (Converter::ToLower(Needle) == Converter::ToLower(Haystack))
		return true;

	return false;
}

string Query_t::GetStringURLPartByNumber(uint8_t Number) {
	if (Number >= PartsData.size())
		return "";

	if (PartsData[Number].first + PartsData[Number].second > strlen(URL))
		return "";

	char Param[PartsData[Number].second];
	memcpy(Param, URL + PartsData[Number].first, PartsData[Number].second);

	string Result(Param, PartsData[Number].second);
	Converter::Trim(Result);

	return Result;
}

const char* Query_t::GetLastURLPartPointer() {
	if (PartsData.size() == 0)
		return NULL;

	return URL + PartsData[PartsData.size() - 1].first;
}


uint8_t Query_t::GetURLPartsCount() {
	return PartsData.size();
}

const char* IRAM_ATTR Query_t::GetBody() {
	if (IsolatedBody != NULL)
		return IsolatedBody;

	if (BodyPointer != NULL && BodyPointer != URL)
		return BodyPointer;

	const char *Result = "";
	return Result;
}

map<string,string> Query_t::GetParams() {
	map<string,string> Result = map<string, string>();

	// check URI first
	const char * pch = strrchr( URL, '?');

	if (pch != NULL)
	{
		int32_t ParamsStringLength = (BodyPointer != NULL && BodyPointer != URL) ? BodyPointer - pch - 2 : strlen(URL) - (pch - URL) - 1;

		if (ParamsStringLength <= 0) ParamsStringLength = 0;

		string ParamsString = string(pch + 1, (uint16_t)ParamsStringLength);

		Converter::Trim(ParamsString);

		if (!ParamsString.empty()) {
			for (string& ParamPair : Converter::StringToVector(ParamsString, "&")) {
				vector<string> Param = Converter::StringToVector(ParamPair, "=");

				if (Param.size() == 2)
					Result[Converter::ToLower(Param[0])] = Converter::StringURLDecode(Param[1]);
				else
					Result[Converter::ToLower(Param[0])] = "";
			}
		}
	}

	return Result;
}

void Query_t::Dump() {
	ESP_LOGD("URL"	, "%s", URL);
	ESP_LOGD("Body"	, "%s", GetBody());

	for (auto& Item : PartsData)
		ESP_LOGD("PartsData", "%d %d", Item.first, Item.second);

	for (auto& Item : GetParams())
		ESP_LOGD("GetParams", "%s %s", Item.first.c_str(), Item.second.c_str());

	switch (Type) {
		case QueryType::GET		: ESP_LOGE("Type", "GET")	; break;
		case QueryType::POST	: ESP_LOGE("Type", "POST")	; break;
		case QueryType::PUT		: ESP_LOGE("Type", "PUT")	; break;
		case QueryType::PATCH	: ESP_LOGE("Type", "PATCH")	; break;
		case QueryType::DELETE	: ESP_LOGE("Type", "DELETE"); break;
		default:
			ESP_LOGE("Type", "NOT SET"); break;
	}
}

