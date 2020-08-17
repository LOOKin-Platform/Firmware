/*
*    API.cpp
*    HTTP Api entry point
*
*/

#include "API.h"

#if CONFIG_FIRMWARE_HOMEKIT_SUPPORT_ADK
#include "HomeKitADK.h"
#endif

#if CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL
#include "HomeKit.h"
#endif

void API::Handle(WebServer_t::Response &Response, Query_t &Query) {
	if (Query.GetURLPartsCount() == 0) {
		map<string,string> Params = Query.GetParams();

		if (!Params.count("summary")) {
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
		else {
			string MQTTChunkHash = "";
			uint16_t ChunkPartID = 0;

			string ResultData = "{ \"Device\":";
			ResultData += Device.RootInfo().ToString() + ",";

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				httpd_resp_set_type	(Query.GetRequest(), HTTPD_TYPE_JSON);

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(), ResultData);
			else {
				MQTTChunkHash = MQTT.StartChunk(Query.MQTTMessageID);
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);
			}

			ResultData = "\"Network\" : ";
			ResultData += Network.RootInfo().ToString() + ",";

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(), ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);


			ResultData = "\"Automation\" : ";
			ResultData += Automation.RootInfo().ToString() + ",";

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(), ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);

			ResultData = "\"Storage\" : { \"Version\" : \"" + Storage.VersionToString() + "\"" + "}, ";

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(), ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);

			ResultData = "\"Sensors\" : [";
			for (int i = 0; i < Sensors.size(); i++) {
				ResultData += " { \"" + Sensors[i]->Name + "\":" + Sensors[i]->EchoSummaryJSON() + "}";
				if (i < Sensors.size() - 1)
					ResultData += ",";
			}
			ResultData += "] ,";

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(), ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);


			ResultData = "\"Commands\" : [";
			for (int i = 0; i < Commands.size(); i++) {
				ResultData += "\"" + Commands[i]->Name + "\"";

				if (i < Commands.size() - 1)
					ResultData += ",";
			}

			ResultData += "]";
			ResultData += "}";

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Query.GetRequest(), ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, Query.MQTTMessageID);

			if (Query.Transport == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::EndChunk(Query.GetRequest());
			else
				MQTT.EndChunk(MQTTChunkHash, Query.MQTTMessageID);

			Response.ResponseCode = WebServer_t::Response::CODE::IGNORE;

			/*
			Result += "\"Log\" : ";
			Log::HandleHTTPRequest(Response, Type, { }, Params);
			Result += Response.Body + "";

			Result += "}";
			*/

			return;
		}

		return;
	}

	if (Query.GetURLPartsCount() > 0) {
		if (Query.CheckURLPart("device"		, 0)) 	Device 		.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("network"	, 0))	Network		.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("automation"	, 0))	Automation	.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("storage"	, 0))	Storage		.HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("data"		, 0))	Data->		HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("sensors"	, 0))	Sensor_t	::HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("commands"	, 0))	Command_t	::HandleHTTPRequest	(Response, Query);
		if (Query.CheckURLPart("log"		, 0))	Log			::HandleHTTPRequest	(Response, Query);

#if (CONFIG_FIRMWARE_HOMEKIT_SUPPORT_ADK || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_RESTRICTED || CONFIG_FIRMWARE_HOMEKIT_SUPPORT_SDK_FULL)
    	if (Query.GetURLPartsCount() == 2)
    	{
    		if (Query.CheckURLPart("homekit", 0)) {
    			if (Query.CheckURLPart("refresh", 1)) {
    				HomeKit::AppServerRestart();
    				Response.SetSuccess();
    				return;
    			}
    			else if (Query.CheckURLPart("reset", 1)) {
    				HomeKit::ResetData();
    				Response.SetSuccess();
    				return;
    			}
    		}
    	}
#endif
		// Fixes null string at some APIs
		if (Query.GetURLPartsCount() == 2 && (Query.CheckURLPart("device", 0)) && (Query.CheckURLPart("name", 1)))
			return;

		if (Query.GetURLPartsCount() == 2 && (Query.CheckURLPart("network", 0)) && (Query.CheckURLPart("currentssid", 1)))
			return;

		// ECO mode test
		if (Query.GetURLPartsCount() > 1 && (Query.CheckURLPart("eco", 0)))
		{
			PowerManagement::SetIsActive(Query.GetStringURLPartByNumber(1) == "on");
			Response.SetSuccess();
		}
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

