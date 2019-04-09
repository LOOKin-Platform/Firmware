/*
*    API.cpp
*    HTTP Api entry point
*
*/

#include "API.h"

void ADCHandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params);

void API::Handle(WebServer_t::Response &Response, Query_t Query) {
  return API::Handle(Response, Query.Type, Query.RequestedUrlParts, Query.Params, Query.RequestBody);
}

void API::Handle(WebServer_t::Response &Response, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody) {
	if (URLParts.size() == 0) {

		if (WiFi.GetMode() == WIFI_MODE_STA_STR) {
			Response.ResponseCode	= WebServer_t::Response::CODE::OK;
			Response.ContentType   = WebServer_t::Response::TYPE::PLAIN;
			Response.Body			= "OK";
		}
		else {
			Response.ResponseCode	= WebServer_t::Response::CODE::OK;
			Response.ContentType	= WebServer_t::Response::TYPE::HTML;
			Response.Body			= WebServer.GetSetupPage();
		}

		return;
	}

	if (URLParts.size() > 0) {
		string APISection = URLParts[0];
		URLParts.erase(URLParts.begin(), URLParts.begin() + 1);

		if (APISection == "device")		Device.HandleHTTPRequest(Response, Type, URLParts, Params);
		if (APISection == "network")	Network.HandleHTTPRequest(Response, Type, URLParts, Params);
		if (APISection == "automation")	Automation.HandleHTTPRequest(Response, Type, URLParts, Params, RequestBody);
		if (APISection == "storage")	Storage.HandleHTTPRequest(Response, Type, URLParts, Params, RequestBody);
		if (APISection == "sensors")	Sensor_t::HandleHTTPRequest(Response, Type, URLParts, Params);
		if (APISection == "commands")	Command_t::HandleHTTPRequest(Response, Type, URLParts, Params);
		if (APISection == "log")		Log::HandleHTTPRequest(Response, Type, URLParts, Params);

		// обработка алиасов комманд
		if (URLParts.size() == 0 && Params.size() == 0) {
			if (APISection == "on")
				switch (Device.Type.Hex) {
				case Settings.Devices.Plug:
					Command_t::HandleHTTPRequest(Response, Type, { "switch", "on" }, map<string,string>() );
					break;
				}

			if (APISection == "off")
				switch (Device.Type.Hex) {
				case Settings.Devices.Plug:
					Command_t::HandleHTTPRequest(Response, Type, { "switch", "off" }, map<string,string>() );
					break;
				}

			if (APISection == "status")
				switch (Device.Type.Hex) {
					case Settings.Devices.Plug:
						Sensor_t::HandleHTTPRequest(Response, Type, { "switch" }, map<string,string>());
						break;
					case Settings.Devices.Remote:
						Sensor_t::HandleHTTPRequest(Response, Type, { "ir" }, map<string,string>());
						break;
				}
		}

		// Fixes null string at some APIs
		if (APISection == "device" && URLParts[0] == "name" && URLParts.size() == 1)
			return;

		if (APISection == "network" && URLParts[0] == "currentssid" && URLParts.size() == 1)
			return;


	}

	if (Response.Body == "" && Response.ResponseCode == WebServer_t::Response::CODE::OK) {
		Response.ResponseCode  = WebServer_t::Response::CODE::INVALID;
		Response.ContentType   = WebServer_t::Response::TYPE::PLAIN;
		Response.Body          = "The request was incorrectly formatted";
	}
}
