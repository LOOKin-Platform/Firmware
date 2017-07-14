using namespace std;

#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <string>

#include "include/Query.h"
#include "include/Tools.h"

Query_t::Query_t(char *buf) {
    Type              = NONE;
    RequestedUrl      = "";
    RequestHeader     = "";
    SrcRequest        = string(buf);

    FillParams(string(buf));
}

void Query_t::FillParams(string Query) {

  vector<string> ProtocolParts = DivideStrBySymbol(Query,' ');

  if (ProtocolParts.size() > 0)
      Type = (ProtocolParts[0] == "GET") ? GET : POST;

  if (ProtocolParts.size() > 1) {
    vector<string> QueryParts = DivideStrBySymbol(ProtocolParts[1], '?');

    if (QueryParts.size() > 0) {
      RequestedUrl      = QueryParts[0];

      // разбиваем строку запроса на массив пути с переводом в нижний регистр
      for (string& part : DivideStrBySymbol(QueryParts[0], '/'))
      {
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
      vector<string>RNDivide = DivideStrBySymbol(SrcRequest, '\n');

      if (RNDivide.size() > 0)
        ParamStr = RNDivide[RNDivide.size() - 1];
    }

    Tools::Trim(ParamStr);

    if (!ParamStr.empty()) {

      vector<string> Param;

      for (string& ParamPair : DivideStrBySymbol(ParamStr, '&')) {
        Param = DivideStrBySymbol(ParamPair, '=');

        if (Param.size() == 2) {
          transform(Param[0].begin(), Param[0].end(), Param[0].begin(), ::tolower);
          Params[Param[0]] = Param[1];
        }
      }
    }
  }

  if (ProtocolParts.size() > 2)
    RequestHeader = ProtocolParts[2];
}

vector<string> Query_t::DivideStrBySymbol(string SourceStr, char Divider) {

  vector<string> Result;

  istringstream f(SourceStr);
  string s;

  while (getline(f, s, Divider))
      Result.push_back(s);

  return Result;
}
