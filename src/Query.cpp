using namespace std;

#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <string>

#include "Query.h"
#include "Tools.h"

Query_t::Query_t(char *buf) {
    Type              = NONE;
    RequestedUrl      = "";
    RequestHeader     = "";
    SrcRequest        = string(buf);

    FillParams(string(buf));
}

void Query_t::FillParams(string Query) {

  vector<string> ProtocolParts = Tools::DivideStrBySymbol(Query,' ');

  if (ProtocolParts.size() > 0)
      Type = (ProtocolParts[0] == "GET") ? GET : POST;

  if (ProtocolParts.size() > 1) {
    vector<string> QueryParts = Tools::DivideStrBySymbol(ProtocolParts[1], '?');

    if (QueryParts.size() > 0) {
      RequestedUrl      = QueryParts[0];

      // разбиваем строку запроса на массив пути с переводом в нижний регистр
      for (string& part : Tools::DivideStrBySymbol(QueryParts[0], '/')) {
        transform(part.begin(), part.end(), part.begin(), ::tolower);
        if (part != "") RequestedUrlParts.push_back( part );
      }
    }

    string ParamStr = "";
    // Параметры для GET запросов требуется получать из URL
    if (Type == GET && QueryParts.size() > 1)
      ParamStr = QueryParts[1];
    // Для всех остальных типов запросов - из тела запроса
    else {
      vector<string>RNDivide = Tools::DivideStrBySymbol(SrcRequest, '\n');

      if (RNDivide.size() > 0)
        ParamStr = RNDivide[RNDivide.size() - 1];
    }

    Tools::Trim(ParamStr);

    if (!ParamStr.empty()) {

      vector<string> Param;

      for (string& ParamPair : Tools::DivideStrBySymbol(ParamStr, '&')) {
        Param = Tools::DivideStrBySymbol(ParamPair, '=');

        if (Param.size() == 2) {
          transform(Param[0].begin(), Param[0].end(), Param[0].begin(), ::tolower);

          Params[Param[0]] = Query_t::UrlDecode(Param[1]);
        }
      }
    }
  }

  if (ProtocolParts.size() > 2)
    RequestHeader = ProtocolParts[2];
}

string Query_t::UrlDecode(string srcString) {

  string Result = srcString;

  Tools::FindAndReplace(Result, "+", " ");
  Tools::FindAndReplace(Result, "%20", " ");

  return Result;
}
