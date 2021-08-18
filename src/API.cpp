/*
*    API.cpp
*    HTTP Api entry point
*
*/

#include "API.h"
#include <HomeKit.h>

uint32_t API::LastAPQueryTime = 0;

void API::Handle(WebServer_t::Response &Response, Query_t &Query) {

	if (WiFi_t::GetMode() == WIFI_MODE_AP_STR)
		API::LastAPQueryTime = Time::Unixtime();

	//if (WiFi_t::GetMode() == WIFI_MODE_AP_STR)
	//	WiFiUptimeHandler::LastAPQueryTime();

	if (Query.GetURLPartsCount() == 0) {
		map<string,string> Params = Query.GetParams();

			if (WiFi.GetMode() == WIFI_MODE_STA_STR) {
				Response.ResponseCode	= WebServer_t::Response::CODE::OK;
				Response.ContentType	= WebServer_t::Response::TYPE::PLAIN;
				Response.Body			= "OK";
			}
			else {
				Response.ResponseCode	= WebServer_t::Response::CODE::OK;
				Response.ContentType	= WebServer_t::Response::TYPE::HTML;
				Response.Body			= GetSetupPage();
			}

			return;
	}

	if (Query.GetURLPartsCount() == 1 && Query.CheckURLPart("summary", 0))
	{
		ESP_LOGE("SUMMARY", ">>");

		Response.Body = "{\"Device\":" + Device.RootInfo().ToString() +  ",";
		Response.Body += "\"Network\":" + Network.RootInfo().ToString() + ",";
		Response.Body += "\"Automation\":" + Automation.RootInfo().ToString() + ",";
		Response.Body += "\"Storage\":{ \"Version\" : \"" + Storage.VersionToString() + "\"" + "}, ";
		Response.Body += "\"Sensors\":[";
		for (int i = 0; i < Sensors.size(); i++) {
			Response.Body += " { \"" + Sensors[i]->Name + "\":" + Sensors[i]->EchoSummaryJSON() + "}";
			if (i < Sensors.size() - 1)
				Response.Body += ",";
		}
		Response.Body += "],";

		Response.Body += "\"Commands\":[";

		for (int i = 0; i < Commands.size(); i++)
		{
			Response.Body += "\"" + Commands[i]->Name + "\"";

			if (i < Commands.size() - 1)
				Response.Body += ",";
		}

		Response.Body += "],";
		Response.Body += "\"Data\":";
		Response.Body += Data->RootInfo();


		/*
		Result += "\"Log\" : ";
		Log::HandleHTTPRequest(Response, Type, { }, Params);
		Result += Response.Body + "";
		Result += "}";
		*/

		Response.Body += "}";

		Response.ResponseCode	= WebServer_t::Response::CODE::OK;
		//Response.ContentType	= WebServer_t::Response::TYPE::JSON;

		ESP_LOGE("SUMMARY", "<<");

		return;
	}

	if (Query.GetURLPartsCount() > 0) {
		if (Query.CheckURLPart("device"		, 0)) 	Device 		.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("network"	, 0))	Network		.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("automation"	, 0))	Automation	.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("storage"	, 0))	Storage		.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("data"		, 0))	Data		->HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("sensors"	, 0))	Sensor_t	::HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("commands"	, 0))	Command_t	::HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("log"		, 0))	Log			::HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("homekit"	, 0))	HomeKitAPI_t::HandleHTTPRequest	(Response, Query);

		// Fixes null string at some APIs
		if (Query.GetURLPartsCount() == 2 && (Query.CheckURLPart("device", 0)) && (Query.CheckURLPart("name", 1)))
			return;

		if (Query.GetURLPartsCount() == 2 && (Query.CheckURLPart("network", 0)) && (Query.CheckURLPart("currentssid", 1)))
			return;
	}

	if (Response.Body == "" && Response.ResponseCode == WebServer_t::Response::CODE::OK) {
		Response.ResponseCode  = WebServer_t::Response::CODE::INVALID;
		Response.ContentType   = WebServer_t::Response::TYPE::PLAIN;
		Response.Body          = "The request was incorrectly formatted";
	}
}

string API::GetSetupPage() {
	// Prepare index page
	vector<string> SSIDList;

	for (WiFiAPRecord Record : Network.WiFiScannedList)
		SSIDList.push_back("'" + Record.getSSID() + "'");

	string SSIDListString = Converter::VectorToString(SSIDList, ",");

	return SetupPage1 + SSIDListString + SetupPage2 + Device.TypeToString() + " " + Device.IDToString() + SetupPage3;
}

