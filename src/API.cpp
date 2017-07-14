using namespace std;

#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>

#include <esp_log.h>

#include "include/Globals.h"
#include "include/API.h"
#include "include/Device.h"
#include "include/Switch_str.h"

WebServerResponse_t* API::Handle(Query_t Query) {
  return API::Handle(Query.Type, Query.RequestedUrlParts, Query.Params);
}

WebServerResponse_t* API::Handle(QueryType Type, vector<string> URLParts, map<string,string> Params) {

  WebServerResponse_t *Response = new WebServerResponse_t();

  if (URLParts.size() > 0) {

    string APISection = URLParts[0];

    URLParts.erase(URLParts.begin(), URLParts.begin() + 1);

    SWITCH(APISection) {
      CASE("device"):
        Response = Device->HandleHTTPRequest(Type, URLParts, Params);
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

          if (APISection == "on")
            switch (Device->Type) {
              case DeviceType::PLUG:
                Response = Command_t::HandleHTTPRequest(Type, { "switch", "on" }, map<string,string>() );
                break;
            }

          if (APISection == "off")
            switch (Device->Type) {
              case DeviceType::PLUG:
                Response = Command_t::HandleHTTPRequest(Type, { "switch", "off" }, map<string,string>() );
                break;
              }

          if (APISection == "status")
            switch (Device->Type) {
              case DeviceType::PLUG:
                Response = Sensor_t::HandleHTTPRequest(Type, { "switch" }, map<string,string>() );
                break;
              }
          }
      }
  }

  if (Response->Body == "")
  {
    Response->ResponseCode  = WebServerResponse_t::CODE::INVALID;
    Response->ContentType   = WebServerResponse_t::TYPE::PLAIN;
    Response->Body          = "The request was incorrectly formatted";
  }

  return Response;
}
