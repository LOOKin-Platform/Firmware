#ifndef QUERY_H
#define QUERY_H

using namespace std;

#include <string>
#include <vector>
#include <map>

#include "HTTPClient/HTTPClient.h"

class Query_t {
  public:
    QueryType           Type;
    string              RequestHeader;
    string              RequestedUrl;
    vector<string>      RequestedUrlParts;

    string              RequestBody;
    map<string,string>  Params;

    Query_t(char *);

    static string       UrlDecode(string);

  private:
    string              SrcRequest;
    void                FillParams(string);
};

#endif
