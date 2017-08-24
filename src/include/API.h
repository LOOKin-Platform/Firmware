/*
*    API.h
*    HTTP Api entry point
*
*/

#ifndef API_H
#define API_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <map>

#include <esp_log.h>

#include "Globals.h"

#include "Query.h"
#include "WebServer.h"

using namespace std;

class API {
  public:
    static void Handle(WebServerResponse_t* &, Query_t Query);
    static void Handle(WebServerResponse_t* &, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody = "");
};

#endif
