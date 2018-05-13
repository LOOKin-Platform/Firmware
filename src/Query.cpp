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

	Query = Query.substr(Pos+1);

	Pos = Query.find(" ");

	if (Pos == string::npos)
		return;

	vector<string> QueryParts = Converter::StringToVector(Query.substr(0, Pos), "?");

	if (QueryParts.size() > 0) {
		RequestedUrl      = QueryParts[0];

		// разбиваем строку запроса на массив пути с переводом в нижний регистр
		for (string& part : Converter::StringToVector(QueryParts[0], "/")) {
			transform(part.begin(), part.end(), part.begin(), ::tolower);
			if (part != "") RequestedUrlParts.push_back( part );
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

				if (Param.size() == 2) Params[Converter::ToLower(Param[0])] = Query_t::UrlDecode(Param[1]);
			}
		}
	}
	else { 	// Для всех остальных типов запросов - из тела запроса в JSON форме
		Pos = Query.rfind("\r\n");

		if (Pos != string::npos) {
			Query = Query.substr(Pos+1);
			Converter::StringMove(RequestBody, Query);
		}

		if (RequestBody != "") {
			JSON JSONItem(RequestBody);
			Params = JSONItem.GetItems();
		}
		else
			Params = map<string,string>();
	}
}

string Query_t::UrlDecode(string srcString) {
	string Result = srcString;

	Converter::FindAndReplace(Result, "+", " ");
	Converter::FindAndReplace(Result, "%20", " ");

	return Result;
}
