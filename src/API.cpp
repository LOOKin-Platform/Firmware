/*
*    API.cpp
*    HTTP Api entry point
*
*/

#include "API.h"

void API::Handle(WebServer_t::Response &Response, Query_t &Query, httpd_req_t *Request, WebServer_t::QueryTransportType TransportType, int MsgID) {
	return API::Handle(Response, Query.Type, Query.RequestedUrlParts, Query.Params, Query.RequestBody, Request, TransportType, MsgID);
}

void API::Handle(WebServer_t::Response &Response, QueryType Type, vector<string> &URLParts, map<string,string> Params, string RequestBody, httpd_req_t *Request, WebServer_t::QueryTransportType TransportType, int MsgID) {

	if (URLParts.size() == 0) {
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
		}
		else {
			string MQTTChunkHash = "";
			uint16_t ChunkPartID = 0;

			string ResultData = "{ \"Device\":";
			Device.HandleHTTPRequest(Response, Type, URLParts, Params);
			ResultData += Response.Body + ",";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				httpd_resp_set_type	(Request, HTTPD_TYPE_JSON);

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else {
				MQTTChunkHash = MQTT.StartChunk(MsgID);
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);
			}

			ResultData = "\"Network\" : ";
			Network.HandleHTTPRequest(Response, Type, URLParts, Params);
			ResultData += Response.Body + ",";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);


			ResultData = "\"Automation\" : ";
			Automation.HandleHTTPRequest(Response, Type, URLParts, Params, RequestBody);
			ResultData += Response.Body + ",";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);

			ResultData = "\"Automation\" : ";
			Automation.HandleHTTPRequest(Response, Type, URLParts, Params, RequestBody);
			ResultData += Response.Body + ",";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);

			Storage.HandleHTTPRequest(Response, Type, { "version" }, Params, RequestBody);
			ResultData = "\"Storage\" : { \"Version\" : \"" + Response.Body  + "\"" + "}, ";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);

			ResultData = "\"Sensors\" : [";
			for (int i = 0; i < Sensors.size(); i++) {
				ResultData += " { \"" + Sensors[i]->Name + "\":" + Sensors[i]->EchoSummaryJSON() + "}";
				if (i < Sensors.size() - 1)
					ResultData += ",";
			}
			ResultData += "] ,";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);


			ResultData = "\"Commands\" : [";
			for (int i = 0; i < Commands.size(); i++) {
				ResultData += "\"" + Commands[i]->Name + "\"";

				if (i < Commands.size() - 1)
					ResultData += ",";
			}

			ResultData += "]";
			ResultData += "}";

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::SendChunk(Request, ResultData);
			else
				MQTT.SendChunk(ResultData, MQTTChunkHash, ChunkPartID++, MsgID);

			if (TransportType == WebServer_t::QueryTransportType::WebServer)
				WebServer_t::EndChunk(Request);
			else
				MQTT.EndChunk(MQTTChunkHash, MsgID);

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

	if (URLParts.size() > 0) {
		string APISection = URLParts[0];
		URLParts.erase(URLParts.begin(), URLParts.begin() + 1);

		if (APISection == "device")		Device		.HandleHTTPRequest	(Response, Type, URLParts, Params, Request, TransportType);
		if (APISection == "network")	Network		.HandleHTTPRequest	(Response, Type, URLParts, Params);
		if (APISection == "automation")	Automation	.HandleHTTPRequest	(Response, Type, URLParts, Params, RequestBody);
		if (APISection == "storage")	Storage		.HandleHTTPRequest	(Response, Type, URLParts, Params, RequestBody, Request, TransportType, MsgID);
		if (APISection == "data")		Data->		HandleHTTPRequest	(Response, Type, URLParts, Params, RequestBody, Request, TransportType, MsgID);
		if (APISection == "sensors")	Sensor_t	::HandleHTTPRequest	(Response, Type, URLParts, Params);
		if (APISection == "commands")	Command_t	::HandleHTTPRequest	(Response, Type, URLParts, Params);
		if (APISection == "log")		Log			::HandleHTTPRequest	(Response, Type, URLParts, Params);

	    if (1) {//if (!CONFIG_ESPTOOLPY_FLASHSIZE_4MB) {
	    	if (URLParts.size() == 1 && Params.size() == 0 && HomeKit::IsSupported()) {
	    		if (APISection == "homekit" && URLParts[0] == "refresh")
	    			HomeKit::AppServerRestart();
					Response.SetSuccess();
					return;
	    	}
		}

		// обработка алиасов комманд
		if (URLParts.size() == 0 && Params.size() == 0) {
			if (APISection == "status")
				switch (Device.Type.Hex) {
					case Settings.Devices.Plug:
						Sensor_t::HandleHTTPRequest(Response, Type, { "switch" }, map<string,string>());
						break;
					case Settings.Devices.Remote:
						Sensor_t::HandleHTTPRequest(Response, Type, { "ir" }, map<string,string>());
						break;
					case Settings.Devices.Duo:
						Sensor_t::HandleHTTPRequest(Response, Type, { "multiswitch" }, map<string,string>());
						break;

				}
		}

		// Fixes null string at some APIs
		if (APISection == "device" && URLParts[0] == "name" && URLParts.size() == 1)
			return;

		if (APISection == "network" && URLParts[0] == "currentssid" && URLParts.size() == 1)
			return;

		// ECO mode test
		if (APISection == "eco" && URLParts.size() > 0) {
			PowerManagement::SetIsActive(URLParts[0] == "on");
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

