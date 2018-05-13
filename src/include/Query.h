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

#include "HTTPClient.h"

class Query_t {
  public:
    QueryType           Type;
    string              RequestedUrl;
    vector<string>      RequestedUrlParts;

    string              RequestBody;
    map<string,string>  Params;

    Query_t(string &);

    static string       UrlDecode(string);

  private:
    void                FillParams(string &);
};

#endif
