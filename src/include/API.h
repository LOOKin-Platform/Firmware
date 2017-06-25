#ifndef API_H
#define API_H

using namespace std;

#include <string>
#include <vector>
#include <map>

#include "Query.h"

class API {
  public:
    string NVSAreaName;

    static string Handle(Query_t Query);
    static string Handle(QueryType Type, vector<string> URLParts, map<string,string> Params);
};

#endif
