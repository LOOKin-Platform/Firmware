/*
*    WebServer.cpp
*    Class for handling TCP and UDP connections
*
*/

using namespace std;

#include <stdio.h>
#include <string.h>

#include "Globals.h"
#include "WebServer.h"
#include "Query.h"
#include "API.h"

#include <esp_log.h>

static char tag[] = "WebServer";

bool WebServer_t::UDPServerStopFlag 			= false;
TaskHandle_t WebServer_t::UDPListenerTaskHandle	= NULL;

httpd_handle_t WebServer_t::HTTPServerHandle 	= NULL;

static vector<uint16_t> UDPPorts 				= { Settings.WiFi.UPDPort };
QueueHandle_t WebServer_t::UDPBroadcastQueue 	= FreeRTOS::Queue::Create(Settings.WiFi.UDPBroadcastQueue.Size, sizeof(UDPBroacastQueueItem));

WebServer_t::WebServer_t() {
	UDPListenerTaskHandle   = NULL;
}

esp_err_t WebServer_t::GETHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	string QueryString = "GET " + string(Request->uri) + " ";

	Query_t Query(QueryString);
	API::Handle(Response, Query);

	SendHTTPData(Response, Request);

    return ESP_OK;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
esp_err_t WebServer_t::POSTHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;
    char content[4096];

    string PostData = "";

    if (Request->content_len > 0) {
        size_t recv_size = MIN(Request->content_len, sizeof(content));
        int ret = httpd_req_recv(Request, content, recv_size);

        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                httpd_resp_send_408(Request);

            return ESP_FAIL;
        }

        PostData = content;
        PostData = PostData.substr(0, recv_size);
    }

	string QueryString = "POST " + string(Request->uri) + " \r\n" + PostData;

	Query_t Query(QueryString);
	API::Handle(Response, Query, Request);

	SendHTTPData(Response, Request);

    return ESP_OK;
}

esp_err_t WebServer_t::DELETEHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	string QueryString = "DELETE " + string(Request->uri) + " ";
	Query_t Query(QueryString);
	API::Handle(Response, Query);

	SendHTTPData(Response, Request);
    return ESP_OK;
}

void WebServer_t::SendHTTPData(WebServer_t::Response& Response, httpd_req_t *Request) {
	WebServer_t::SetHeaders(Response, Request);
	httpd_resp_send(Request, Response.Body.c_str(), HTTPD_RESP_USE_STRLEN);
}

void WebServer_t::SetHeaders(WebServer_t::Response &Response, httpd_req_t *Request) {
	switch (Response.ResponseCode) {
		case Response::CODE::INVALID  	: httpd_resp_set_status	(Request, HTTPD_400); break;
		case Response::CODE::ERROR    	: httpd_resp_set_status	(Request, HTTPD_500); break;
		default							: httpd_resp_set_status	(Request, HTTPD_200); break;
	}

	switch (Response.ContentType) {
		case Response::TYPE::JSON  		: httpd_resp_set_type	(Request, HTTPD_TYPE_JSON); break;
		default							: httpd_resp_set_type	(Request, HTTPD_TYPE_TEXT); break;
	}

	//httpd_resp_set_hdr(Request, "Access-Control-Allow-Origin", "*");
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/*",
    .method   = HTTP_GET,
    .handler  = WebServer_t::GETHandler,
    .user_ctx = NULL
};

/* URI handler structure for GET /uri */
httpd_uri_t uri_post = {
    .uri      = "/*",
    .method   = HTTP_POST,
    .handler  = WebServer_t::POSTHandler,
    .user_ctx = NULL
};

/* URI handler structure for GET /uri */
httpd_uri_t uri_delete = {
    .uri      = "/*",
    .method   = HTTP_DELETE,
    .handler  = WebServer_t::DELETEHandler,
    .user_ctx = NULL
};


void WebServer_t::Start() {
	ESP_LOGD(tag, "WebServer -> Start");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn 	= httpd_uri_match_wildcard;
    config.stack_size		= 16384;
    config.lru_purge_enable = true;
    //config.task_priority	= tskIDLE_PRIORITY+5;

    HTTPServerHandle = NULL;

    if (httpd_start(&HTTPServerHandle, &config) == ESP_OK) {
        httpd_register_uri_handler(HTTPServerHandle, &uri_get);
        httpd_register_uri_handler(HTTPServerHandle, &uri_post);
        httpd_register_uri_handler(HTTPServerHandle, &uri_delete);
    }

	if (UDPListenerTaskHandle == NULL)
		UDPListenerTaskHandle   = FreeRTOS::StartTask(UDPListenerTask , "UDPListenerTask" , NULL, 4096);
}

void WebServer_t::Stop() {
	ESP_LOGD(tag, "WebServer -> Stop");

    if (HTTPServerHandle != NULL) {
        httpd_stop(HTTPServerHandle);
        HTTPServerHandle = NULL;
    }

	UDPServerStopFlag 	= true;
}

void WebServer_t::UDPSendBroadcastAlive() {
	UDPSendBroadcast(UDPAliveBody());
}

void WebServer_t::UDPSendBroadcastDiscover() {
	UDPSendBroadcast(UDPDiscoverBody());
}

void WebServer_t::UDPSendBroadcastUpdated(uint8_t SensorID, string Value, uint8_t Repeat) {
	for (int i=0; i < Repeat; i++)
		UDPSendBroadcast(UDPUpdatedBody(SensorID, Value));
}

string WebServer_t::UDPAliveBody() {
	string Message = "Alive!"
			+ Device.IDToString()
			+ ":" + Device.Type.ToHexString()
			+ ":" + (Device.Type.IsBattery() ? "0" : "1")
			+ ":" + Network.IPToString()
			+ ":" + Converter::ToHexString(Automation.CurrentVersion(),8)
			+ ":" + Converter::ToHexString(Storage.CurrentVersion(),4);

	return Settings.WiFi.UDPPacketPrefix + Message;
}

string WebServer_t::UDPDiscoverBody(string ID) {
	string Message = (ID != "") ? ("Discover!" + ID) : "Discover!";
	return Settings.WiFi.UDPPacketPrefix + Message;
}

string WebServer_t::UDPUpdatedBody(uint8_t SensorID, string Value) {
	string Message = "Updated!" + Device.IDToString() + ":" + Converter::ToHexString(SensorID, 2) + ":" + Value;
	return Settings.WiFi.UDPPacketPrefix + Message;
}

void WebServer_t::UDPSendBroadcast(string Message) {
	if (WiFi.IsRunning()
		&& (WiFi.GetMode() == WIFI_MODE_AP_STR || (WiFi.GetMode() == WIFI_MODE_STA_STR && WiFi.IsIPCheckSuccess)))
	{
		struct netconn *Connection;
		struct netbuf *Buffer;
		char *Data;

		if (Message.length() > 128)
			Message = Message.substr(0, 128);

		for (uint16_t Port : UDPPorts) {
			Connection = netconn_new(NETCONN_UDP);
			netconn_connect(Connection, IP_ADDR_BROADCAST, Port);

			Buffer  = netbuf_new();
			Data    = (char *)netbuf_alloc(Buffer, Message.length());
			memcpy (Data, Message.c_str(), Message.length());
			netconn_send(Connection, Buffer);

			netconn_close(Connection);
			netconn_delete(Connection);

			netbuf_free(Buffer);
			netbuf_delete(Buffer); 		// De-allocate packet buffer

			ESP_LOGI(tag, "UDP broadcast \"%s\" sended to port %d", Message.c_str(), Port);
		}
	}
	else
	{
		UDPSendBroadcastQueueAdd(Message);
		ESP_LOGE(tag, "WiFi switched off - can't send UDP message");
		return;
	}
}

void WebServer_t::UDPSendBroacastFromQueue() {
	UDPBroacastQueueItem ItemToSend;

	if (UDPBroadcastQueue != 0)
		while (FreeRTOS::Queue::Receive(UDPBroadcastQueue, &ItemToSend, (TickType_t) Settings.WiFi.UDPBroadcastQueue.BlockTicks)) {
			if (ItemToSend.Updated + Settings.WiFi.UDPBroadcastQueue.CheckGap >= Time::Uptime()) {
				for (int i=0; i<3; i++) {
					UDPSendBroadcast(string(ItemToSend.Message));
					FreeRTOS::Sleep(Settings.WiFi.UDPBroadcastQueue.Pause);
				}
			}
		}
}

void WebServer_t::UDPSendBroadcastQueueAdd(string Message) {
	if (Message.size() > Settings.WiFi.UDPBroadcastQueue.Size)
		Message = Message.substr(0, Settings.WiFi.UDPBroadcastQueue.MaxMessageSize);

	UDPBroacastQueueItem ItemToSend;
	ItemToSend.Updated = Time::Uptime();
	strcpy(ItemToSend.Message, Message.c_str());

	FreeRTOS::Queue::SendToBack(UDPBroadcastQueue, &ItemToSend, (TickType_t) Settings.HTTPClient.BlockTicks );
}

void WebServer_t::UDPListenerTask(void *data) {
	if (!WiFi.IsRunning()) {
		ESP_LOGE(tag, "WiFi switched off - can't start UDP listener task");
		WebServer_t::UDPListenerTaskHandle = NULL;
		FreeRTOS::DeleteTask();
		return;
	}

	UDPServerStopFlag = false;
	ESP_LOGD(tag, "UDPListenerTask Run");

	struct netconn 	*UDPConnection;
	struct netbuf 	*UDPInBuffer, *UDPOutBuffer;
	char 			*UDPInData	, *UDPOutData;
	u16_t inDataLen;
	err_t err;

	UDPConnection = netconn_new(NETCONN_UDP);
	netconn_bind(UDPConnection, IP_ADDR_ANY, Settings.WiFi.UPDPort);

	do {
		err = netconn_recv(UDPConnection, &UDPInBuffer);

		if (err == ERR_OK) {
			netbuf_data(UDPInBuffer, (void * *)&UDPInData, &inDataLen);
			string Datagram = UDPInData;

			ESP_LOGI(tag, "UDP received \"%s\"", Datagram.c_str());

			if(find(UDPPorts.begin(), UDPPorts.end(), UDPInBuffer->port) == UDPPorts.end()) {
				if (UDPPorts.size() == Settings.WiFi.UDPHoldPortsMax) UDPPorts.erase(UDPPorts.begin() + 1);
				UDPPorts.push_back(UDPInBuffer->port);
			}

			UDPOutBuffer = netbuf_new();

			// Redirect UDP message if in access point mode
			//if (WiFi_t::GetMode() == WIFI_MODE_AP_STR)
			//	WebServer->UDPSendBroadcast(Datagram);

			// answer to the Discover query
			if (Datagram == WebServer_t::UDPDiscoverBody() || Datagram == WebServer_t::UDPDiscoverBody(Device.IDToString())) {
				string Answer = WebServer_t::UDPAliveBody();

				UDPOutData = (char *)netbuf_alloc(UDPOutBuffer, Answer.length());
				memcpy (UDPOutData, Answer.c_str(), Answer.length());

				netconn_sendto(UDPConnection, UDPOutBuffer, &UDPInBuffer->addr, UDPInBuffer->port);

				ESP_LOGD(tag, "UDP \"%s\" sended to %s:%u", Answer.c_str(), inet_ntoa(UDPInBuffer->addr), UDPInBuffer->port);
			}

			string AliveText = Settings.WiFi.UDPPacketPrefix + string("Alive!");
			size_t AliveFound = Datagram.find(AliveText);

			if (AliveFound != string::npos) {
				string Alive = Datagram;
				Alive.replace(Alive.find(AliveText),AliveText.length(),"");
				vector<string> Data = Converter::StringToVector(Alive, ":");

				while (Data.size() < 6)
					Data.push_back("");

				Network.DeviceInfoReceived(Data[0], Data[1], Data[2], Data[3], Data[4], Data[5]);
			}

			netbuf_delete(UDPOutBuffer);
			netbuf_delete(UDPInBuffer);
		}
	} while(err == ERR_OK && !UDPServerStopFlag);

	netconn_close(UDPConnection);
	netconn_delete(UDPConnection);

	FreeRTOS::DeleteTask(UDPListenerTaskHandle);
	UDPListenerTaskHandle = NULL;
}

static bool IsSetupPageTemplated = false;
string WebServer_t::GetSetupPage() {
	if (!IsSetupPageTemplated)
	{
		// Prepare index page
	    size_t index = SetupPage.find("{SSID_LIST}");
	    if (index != std::string::npos) {
	    	vector<string> SSIDList;

	    	for (WiFiAPRecord Record : Network.WiFiScannedList)
	    		SSIDList.push_back("'" + Record.getSSID() + "'");

	    	string SSIDListString = Converter::VectorToString(SSIDList, ",");
	    	SetupPage = SetupPage.substr(0, index) + SSIDListString + SetupPage.substr(index + 11);
	    }

	    index = SetupPage.find("{CURRENT_DEVICE}");
	    if (index != std::string::npos)
	    	SetupPage = SetupPage.substr(0, index) + Device.TypeToString() + " " + Device.IDToString() + SetupPage.substr(index + 16);

	    IsSetupPageTemplated = true;
	}

	return SetupPage;
}

/*
*    WebServer_t::Response
*    class for building user-friendly browser answer
*
*/

WebServer_t::Response::Response() {
	ResponseCode = CODE::OK;
	ContentType  = TYPE::JSON;
}

void WebServer_t::Response::SetSuccess() {
	ResponseCode  = CODE::OK;
	ContentType   = TYPE::JSON;
	Body          = "{\"success\" : \"true\"}";
}

void WebServer_t::Response::SetFail() {
	ResponseCode  = CODE::ERROR;
	ContentType   = TYPE::JSON;
	Body          = "{\"success\" : \"false\"}";
}

void WebServer_t::Response::SetInvalid() {
	ResponseCode  = CODE::INVALID;
	ContentType   = TYPE::JSON;
	Body          = "{\"success\" : \"false\"}";
}

void WebServer_t::Response::Clear() {
	Body = "";
	ResponseCode = OK;
	ContentType = JSON;
}

uint16_t WebServer_t::Response::CodeToInt() {
	switch (ResponseCode) {
		case Response::CODE::INVALID  	: return 400; break;
		case Response::CODE::ERROR    	: return 500; break;
		default							: return 200; break;
	}
}

