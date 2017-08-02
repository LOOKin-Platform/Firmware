#ifndef API_H
#define API_H

using namespace std;

#include <string>
#include <vector>
#include <map>

#include "Query.h"
#include "WebServer.h"

class API {
  public:
    string NVSAreaName;

    static void Handle(WebServerResponse_t* &, Query_t Query);
    static void Handle(WebServerResponse_t* &, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody = "");
};

#endif
