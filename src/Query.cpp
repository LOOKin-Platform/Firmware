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

Query_t::Query_t(string &buf) {
	Type              = NONE;
    RequestedUrl      = "";
    RequestBody       = "";

    FillParams(buf);
}

void Query_t::FillParams(string &Query) {
	size_t Pos = Query.find(" ");

	if (Pos > 6 || Pos == string::npos)
		return;

	if (Query.substr(0, Pos) == "GET")		Type = GET;
	if (Query.substr(0, Pos) == "POST")		Type = POST;
	if (Query.substr(0, Pos) == "DELETE")	Type = DELETE;
	if (Query.substr(0, Pos) == "PUT")		Type = PUT;
	if (Query.substr(0, Pos) == "PATCH")	Type = PATCH;

	Query = Query.substr(Pos+1);

	Pos = Query.find(" ");

	if (Pos == string::npos)
		Pos = Query.length();

	vector<string> QueryParts = Converter::StringToVector(Query.substr(0, Pos), "?");

	if (QueryParts.size() > 0) {
		RequestedUrl      = QueryParts[0];

		// разбиваем строку запроса на массив пути с переводом в нижний регистр
		for (string& part : Converter::StringToVector(QueryParts[0], "/")) {
			transform(part.begin(), part.end(), part.begin(), ::tolower);
			if (part != "") RequestedUrlParts.push_back( Converter::StringURLDecode(part) );
		}
	}

	string ParamStr = "";
	// Параметры для GET запросов требуется получать из URL
	if (Type == GET && QueryParts.size() > 1) {
		ParamStr = QueryParts[1];
		Converter::Trim(ParamStr);

		if (!ParamStr.empty()) {
			vector<string> Param;

			for (string& ParamPair : Converter::StringToVector(ParamStr, "&")) {
				Param = Converter::StringToVector(ParamPair, "=");

				if (Param.size() == 2)
					Params[Converter::ToLower(Param[0])] = Converter::StringURLDecode(Param[1]);
				else
					Params[Converter::ToLower(Param[0])] = "";
			}
		}
	}
	else { 	// Для всех остальных типов запросов - из тела запроса в JSON форме
		Pos = Query.rfind("\r\n");

		if (Pos != string::npos) {
			Query = Query.substr(Pos+2);
			Converter::StringMove(RequestBody, Query);
		}
		else {
			Pos = Query.rfind('\n');
			if (Pos != string::npos) {
				Query = Query.substr(Pos+1);
				Converter::StringMove(RequestBody, Query);
			}
		}

		if (RequestBody.size() > 2) {
			JSON JSONItem(RequestBody);

			if (JSONItem.GetType() == JSON::RootType::Object)
				Params = JSONItem.GetItems();
		}
		else
			Params = map<string,string>();

	}
}
