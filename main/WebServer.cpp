/*
*    WebServer.cpp
*    Class for handling TCP and UDP connections
*
*/

using namespace std;

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

string WebServer_t::AllowOriginHeader 			= "";

WebServer_t::WebServer_t() {
	UDPListenerTaskHandle   = NULL;
}

esp_err_t WebServer_t::GETHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	Query_t Query(Request, QueryType::GET);
	API::Handle(Response, Query);

	SendHTTPData(Response, Request);

	Query.Cleanup();

	return ESP_OK;
}

esp_err_t WebServer_t::POSTHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	Query_t Query(Request, QueryType::POST);
	API::Handle(Response, Query);
	SendHTTPData(Response, Request);
	Query.Cleanup();

    return ESP_OK;
}

esp_err_t WebServer_t::PUTHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	Query_t Query(Request, QueryType::PUT);
	API::Handle(Response, Query);
	SendHTTPData(Response, Request);
	Query.Cleanup();

    return ESP_OK;
}

esp_err_t WebServer_t::DELETEHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	Query_t Query(Request, QueryType::DELETE);
	API::Handle(Response, Query);
	SendHTTPData(Response, Request);
	Query.Cleanup();

    return ESP_OK;
}

esp_err_t WebServer_t::PATCHHandler(httpd_req_t *Request) {
	WebServer_t::Response Response;

	Query_t Query(Request, QueryType::PATCH);
	API::Handle(Response, Query);

	Query.Cleanup();

	SendHTTPData(Response, Request);

    return ESP_OK;
}

void WebServer_t::SendHTTPData(WebServer_t::Response& Response, httpd_req_t *Request, bool TerminateSession) {
	if (Response.ResponseCode != WebServer_t::Response::CODE::IGNORE)
	{
		ESP_LOGE("REQUEST", "%s", Response.Body.c_str());
		WebServer_t::SetHeaders(Response, Request);
		httpd_resp_send(Request, Response.Body.c_str(), Response.Body.size());

		if (TerminateSession)
			httpd_sess_trigger_close(Request->handle, httpd_req_to_sockfd(Request));
	}
}

void WebServer_t::SendChunk(httpd_req_t *Request, string Part) {
	::httpd_resp_sendstr_chunk(Request, Part.c_str());
}

void WebServer_t::EndChunk(httpd_req_t *Request) {
	::httpd_resp_sendstr_chunk(Request, NULL);
}

void WebServer_t::SetHeaders(WebServer_t::Response &Response, httpd_req_t *Request) {
	switch (Response.ResponseCode) {
		case Response::CODE::INVALID  	: httpd_resp_set_status	(Request, HTTPD_400); break;
		case Response::CODE::ERROR    	: httpd_resp_set_status	(Request, HTTPD_500); break;
		default							: httpd_resp_set_status	(Request, HTTPD_200); break;
	}

	switch (Response.ContentType) {
		case Response::TYPE::JSON  		: httpd_resp_set_type	(Request, "application/json;charset=utf-8"); break;
		default							: httpd_resp_set_type	(Request, "text/html;charset=utf-8"); break;
	}

	if (AllowOriginHeader != "")
		httpd_resp_set_hdr(Request, "Access-Control-Allow-Origin", AllowOriginHeader.c_str());
}

httpd_uri_t uri_root_get 			= { .uri = "/"				, .method = HTTP_GET		, .handler  = WebServer_t::GETHandler	, .user_ctx = NULL};
httpd_uri_t uri_get 				= { .uri = "/*"				, .method = HTTP_GET		, .handler  = WebServer_t::GETHandler	, .user_ctx = NULL};
httpd_uri_t uri_post 				= { .uri = "/*"				, .method = HTTP_POST		, .handler  = WebServer_t::POSTHandler	, .user_ctx = NULL};
httpd_uri_t uri_put 				= { .uri = "/*"				, .method = HTTP_PUT		, .handler  = WebServer_t::PUTHandler	, .user_ctx = NULL};
httpd_uri_t uri_delete 				= { .uri = "/*"				, .method = HTTP_DELETE		, .handler  = WebServer_t::DELETEHandler, .user_ctx = NULL};

void WebServer_t::HTTPStart() {
	ESP_LOGD(tag, "HTTPServer -> Start");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn 	= httpd_uri_match_wildcard;
    config.stack_size		= 8192;//4096;//12288;//16384;//20000;
    config.lru_purge_enable = true;
    config.task_priority	= tskIDLE_PRIORITY + 7;
    config.max_uri_handlers = 30;
    config.ctrl_port		= 32768;

    HTTPServerHandle		 = NULL;

    if (httpd_start(&HTTPServerHandle, &config) == ESP_OK) {
    	RegisterHandlers(HTTPServerHandle);
    }
}

void WebServer_t::HTTPStop() {
	ESP_LOGD(tag, "HTTPServer -> Stop");

    if (HTTPServerHandle != NULL) {
        httpd_stop(HTTPServerHandle);
        HTTPServerHandle = NULL;
    }
}

void WebServer_t::UDPStart() {
	ESP_LOGD(tag, "UDPServer -> Start");

	if (UDPListenerTaskHandle == NULL) {
		UDPListenerTaskHandle = FreeRTOS::StartTask(UDPListenerTask , "UDPListenerTask" , NULL, 3072, 5);
	}
}

void WebServer_t::UDPStop() {
	ESP_LOGD(tag, "UDPServer -> Stop");

	UDPServerStopFlag 	= true;
}

void WebServer_t::RegisterHandlers(httpd_handle_t ServerHandle) {
	ESP_LOGE("RegisterHandlers", "start");

	if (ServerHandle == NULL)
		ESP_LOGE("RegisterHandlers", "empty");

	httpd_register_uri_handler(ServerHandle, &uri_root_get);
	httpd_register_uri_handler(ServerHandle, &uri_get);
	httpd_register_uri_handler(ServerHandle, &uri_post);
	httpd_register_uri_handler(ServerHandle, &uri_put);
	httpd_register_uri_handler(ServerHandle, &uri_delete);
}

void WebServer_t::UnregisterHandlers(httpd_handle_t ServerHandle) {
	httpd_unregister_uri_handler(ServerHandle, "/"				, HTTP_GET);
	httpd_unregister_uri_handler(ServerHandle, "/*"				, HTTP_GET);
	httpd_unregister_uri_handler(ServerHandle, "/*"				, HTTP_POST);
	httpd_unregister_uri_handler(ServerHandle, "/*"				, HTTP_PUT);
	httpd_unregister_uri_handler(ServerHandle, "/*"				, HTTP_DELETE);
}

void WebServer_t::UDPSendBroadcastAlive() {
	UDPSendBroadcast(UDPAliveBody());
}

void WebServer_t::UDPSendBroadcastDiscover() {
	UDPSendBroadcast(UDPDiscoverBody());
}

void WebServer_t::UDPSendBroadcastUpdated(uint8_t SensorID, string EventID, uint8_t Repeat, string Operand) {
	for (int i=0; i < Repeat; i++)
		UDPSendBroadcast(UDPUpdatedBody(Converter::ToHexString(SensorID,2), EventID, Operand));
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

string WebServer_t::UDPUpdatedBody(string SensorID, string Value, string Operand) {
	string Message = "Updated!" + Device.IDToString() + ":" + SensorID + ":" + Value;

	if (Operand != "") Message += ":" + Operand;

	return Settings.WiFi.UDPPacketPrefix + Message;
}

void WebServer_t::UDPSendBroadcast(string Message, bool IsScheduled) {
	if (WiFi.IsRunning()
		&& (WiFi.GetMode() == WIFI_MODE_AP_STR || (WiFi.GetMode() == WIFI_MODE_STA_STR && WiFi.IsIPCheckSuccess)))
	{
	    char addr_str[128];
	    int addr_family;
	    int ip_protocol;

		if (Message.length() > 128)
			Message = Message.substr(0, 128);

		for (uint16_t Port : UDPPorts) {
			struct sockaddr_in dest_addr;
			dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(Port);
			addr_family = AF_INET;
			ip_protocol = IPPROTO_IP;

			inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

			int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
			if (sock < 0) {
				ESP_LOGE(tag, "Unable to create socket: errno %d", errno);
				break;
			}
			ESP_LOGI(tag, "Socket created, sending to port %d", Port);

			int option = 1;

			if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,(char*)&option,sizeof(option)) < 0)
				ESP_LOGE(tag, "setsockopt SO_REUSEADDR failed");

			if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST,(char*)&option,sizeof(option)) < 0)
				ESP_LOGE(tag, "setsockopt SO_BROADCAST failed");

			int err = sendto(sock, Message.c_str(), Message.length(), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

			if (err < 0) {
				ESP_LOGE(tag, "Error occurred during sending: errno %d", errno);
				break;
			}

			ESP_LOGI(tag, "UDP broadcast \"%s\" sended to port %d, sock: %d", Message.c_str(), Port, sock);

	        if (sock != -1) {
	            shutdown(sock, 2);
	            close(sock);
	        }
		}
	}
	else
	{
		if (IsScheduled)
			UDPSendBroadcastQueueAdd(Message);

		return;
	}
}

void WebServer_t::UDPSendBroadcastFromQueue() {
	UDPBroacastQueueItem ItemToSend;

	if (UDPBroadcastQueue != 0)
		while (FreeRTOS::Queue::Receive(UDPBroadcastQueue, &ItemToSend, (TickType_t) Settings.WiFi.UDPBroadcastQueue.BlockTicks)) {
			if (ItemToSend.Updated + Settings.WiFi.UDPBroadcastQueue.CheckGap >= ::Time::Uptime()) {
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
	ItemToSend.Updated = ::Time::Uptime();
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

    char 	rx_buffer[128];
    char	addr_str[128];
    int 	addr_family;
    int 	ip_protocol;

    do 
	{
    	struct sockaddr_in dest_addr;
    	dest_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
    	dest_addr.sin_family 		= AF_INET;
    	dest_addr.sin_port 			= htons(Settings.WiFi.UPDPort);
    	addr_family 				= AF_INET;
    	ip_protocol 				= IPPROTO_IP;

    	inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    	int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    	if (sock < 0) {
    		ESP_LOGE(tag, "Unable to create socket: errno %d", errno);
    		break;
    	}

    	int option = 1;
        if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,(char*)&option,sizeof(option)) < 0)
        	ESP_LOGE(tag, "setsockopt SO_REUSEADDR failed");

    	int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    	if (err < 0) {
    		ESP_LOGE(tag, "Socket unable to bind: errno %d", errno);
    	}

    	do
    	{
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            if (len < 0) {
                ESP_LOGE(tag, "recvfrom failed: errno %d", errno);
                break;
            }
            else
            {
                // Get the sender's ip address as string
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...

                string Datagram = rx_buffer;
    			ESP_LOGE(tag, "UDP received \"%s\" from port %d", Datagram.c_str(), ntohs(source_addr.sin6_port));

    			if(find(UDPPorts.begin(), UDPPorts.end(), ntohs(source_addr.sin6_port)) == UDPPorts.end()) {
    				if (UDPPorts.size() == Settings.WiFi.UDPHoldPortsMax)
    					UDPPorts.erase(UDPPorts.begin() + 1);
    				UDPPorts.push_back(ntohs(source_addr.sin6_port));
    			}

    			// Redirect UDP message if in access point mode
    			//if (WiFi_t::GetMode() == WIFI_MODE_AP_STR)
    			//	WebServer->UDPSendBroadcast(Datagram);

    			// answer to the Discover query

    			if (Datagram.size() > 8)
    				if (Datagram.find("LOOK.in") == 0)
    					Datagram = Settings.WiFi.UDPPacketPrefix + Datagram.substr(8);

    			if (Datagram == WebServer_t::UDPDiscoverBody() || Datagram == WebServer_t::UDPDiscoverBody(Device.IDToString())) {
    				string Answer = WebServer_t::UDPAliveBody();

    				int err = sendto(sock, Answer.c_str(), Answer.length(), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
    	            if (err < 0) {
    	                ESP_LOGE(tag, "Error occurred during sending: errno %d", errno);
    	                break;
    	            }

    				ESP_LOGD(tag, "UDP \"%s\" sended to port %u", Answer.c_str(), ntohs(source_addr.sin6_port));
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
    		}
    	} while(!UDPServerStopFlag);

        if (sock != -1) {
            shutdown(sock, 0);
            close(sock);
        }

    } while(!UDPServerStopFlag);

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

	    	for (WiFiScannedAPListItem Record : Network.WiFiScannedList)
	    		SSIDList.push_back("'" + Record.SSID + "'");

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

void WebServer_t::Response::SetSuccess() {
	ResponseCode  	= CODE::OK;
	ContentType   	= TYPE::JSON;
	Body          	= "{\"success\" : \"true\"}";
}

void WebServer_t::Response::SetFail(string Reason) {
	ResponseCode  	= CODE::ERROR;
	ContentType   	= TYPE::JSON;

	if (Reason == "")
		Body = "{\"success\" : \"false\"}";
	else
		Body = "{\"success\" : \"false\", \"message\" : \"" + Reason + "\"}";
}

void WebServer_t::Response::SetInvalid(string Reason) {
	ResponseCode  	= CODE::INVALID;
	ContentType   	= TYPE::JSON;

	if (Reason == "")
		Body = "{\"success\" : \"false\"}";
	else
		Body = "{\"success\" : \"false\", \"message\" : \"" + Reason + "\"}";
}

void WebServer_t::Response::Clear() {
	Body			= "";
	ResponseCode 	= OK;
	ContentType 	= JSON;
}

uint16_t WebServer_t::Response::CodeToInt() {
	switch (ResponseCode) {
		case Response::CODE::INVALID  	: return 400; break;
		case Response::CODE::ERROR    	: return 500; break;
		default							: return 200; break;
	}
}

