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

#include "JSONWrapper.h"
#include "Query.h"
#include "Converter.h"

#include <esp_log.h>

Query_t::Query_t(string buf) {
    Type              = NONE;
    RequestedUrl      = "";
    RequestHeader     = "";
    RequestBody       = "";
    SrcRequest        = buf;

    FillParams(buf);
}

void Query_t::FillParams(string Query) {
  vector<string> ProtocolParts = Converter::StringToVector(Query, " ");

  if (ProtocolParts.size() > 0) {
    if (ProtocolParts[0] == "GET")    Type = GET;
    if (ProtocolParts[0] == "POST")   Type = POST;
    if (ProtocolParts[0] == "DELETE") Type = DELETE;
  }

  if (ProtocolParts.size() > 1) {
    vector<string> QueryParts = Converter::StringToVector(ProtocolParts[1], "?");

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

          if (Param.size() == 2)
            Params[Converter::ToLower(Param[0])] = Query_t::UrlDecode(Param[1]);
        }
      }
    }
    // Для всех остальных типов запросов - из тела запроса в JSON форме
    else {
      vector<string>RNDivide = Converter::StringToVector(SrcRequest, "\r\n");
      RequestBody = RNDivide[RNDivide.size() - 1];

      JSON JSONItem(RequestBody);
      Params = JSONItem.GetItems();
    }
  }

  if (ProtocolParts.size() > 2)
    RequestHeader = ProtocolParts[2];
}

string Query_t::UrlDecode(string srcString) {

  string Result = srcString;

  Converter::FindAndReplace(Result, "+", " ");
  Converter::FindAndReplace(Result, "%20", " ");

  return Result;
}
