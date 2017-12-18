/*
*    API.cpp
*    HTTP Api entry point
*
*/

#include "API.h"

void API::Handle(WebServer_t::Response &Response, Query_t Query) {
  return API::Handle(Response, Query.Type, Query.RequestedUrlParts, Query.Params, Query.RequestBody);
}

void API::Handle(WebServer_t::Response &Response, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody) {

  if (URLParts.size() == 0) {
    Response.ResponseCode  = WebServer_t::Response::CODE::OK;
    Response.ContentType   = WebServer_t::Response::TYPE::PLAIN;
    Response.Body          = "OK";
    return;
  }

  if (URLParts.size() > 0) {
    string APISection = URLParts[0];
    URLParts.erase(URLParts.begin(), URLParts.begin() + 1);

    if (APISection == "device")     Device->HandleHTTPRequest(Response, Type, URLParts, Params);
    if (APISection == "network")    Network->HandleHTTPRequest(Response, Type, URLParts, Params);
    if (APISection == "automation") Automation->HandleHTTPRequest(Response, Type, URLParts, Params, RequestBody);
    if (APISection == "sensors")    Sensor_t::HandleHTTPRequest(Response, Type, URLParts, Params);
    if (APISection == "commands")   Command_t::HandleHTTPRequest(Response, Type, URLParts, Params);
    if (APISection == "log")        Log::HandleHTTPRequest(Response, Type, URLParts, Params);

    // обработка алиасов комманд
    if (URLParts.size() == 0 && Params.size() == 0) {

      if (APISection == "on")
        switch (Device->Type->Hex) {
          case DEVICE_TYPE_PLUG_HEX:
            Command_t::HandleHTTPRequest(Response, Type, { "switch", "on" }, map<string,string>() );
            break;
        }

      if (APISection == "off")
        switch (Device->Type->Hex) {
          case DEVICE_TYPE_PLUG_HEX:
            Command_t::HandleHTTPRequest(Response, Type, { "switch", "off" }, map<string,string>() );
            break;
          }

      if (APISection == "status")
        switch (Device->Type->Hex) {
          case DEVICE_TYPE_PLUG_HEX:
            Sensor_t::HandleHTTPRequest(Response, Type, { "switch" }, map<string,string>());
            break;
        }
    }
  }

  if (Response.Body == "" && Response.ResponseCode == WebServer_t::Response::CODE::OK) {
    Response.ResponseCode  = WebServer_t::Response::CODE::INVALID;
    Response.ContentType   = WebServer_t::Response::TYPE::PLAIN;
    Response.Body          = "The request was incorrectly formatted";
  }
}
