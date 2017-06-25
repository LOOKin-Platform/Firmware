using namespace std;

#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>

#include "include/Globals.h"
#include "include/API.h"
#include "include/Device.h"
#include "include/Switch_str.h"

string API::Handle(Query_t Query) {
  return API::Handle(Query.Type, Query.RequestedUrlParts, Query.Params);
}

string API::Handle(QueryType Type, vector<string> URLParts, map<string,string> Params) {

  string Response = "";

  if (URLParts.size() > 0) {
    string APISection = URLParts[0];
    URLParts.erase(URLParts.begin(), URLParts.begin() + 1);

    SWITCH(APISection) {
      CASE("device"):
        Response = Device.HandleHTTPRequest(Type, URLParts, Params);
        break;
      CASE("sensors"):
        Response = Sensor_t::HandleHTTPRequest(Type, URLParts, Params);
        break;
      CASE("commands"):
        Response = Command_t::HandleHTTPRequest(Type, URLParts, Params);
        break;
      DEFAULT:
        // обработка алиасов комманд
        if (URLParts.size() == 0 && Params.size() == 0) {
          if (APISection == "on"  && Device.Type == DeviceType::PLUG) Command_t::GetCommandByName("switch").Execute("on");
          if (APISection == "off" && Device.Type == DeviceType::PLUG) Command_t::GetCommandByName("switch").Execute("off");
          }
      }
  }

  if (Response == "") Response = "The request was incorrectly formatted";

  return Response;
}
