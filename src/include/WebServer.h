/*
*    WebServer.cpp
*    Class for handling TCP and UDP connections
*
*/
#ifndef WEBSERVER_H
#define WEBSERVER_H

using namespace std;

#include <string>

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/err.h"

#include "esp_http_server.h"

#include "FreeRTOSWrapper.h"

struct UDPBroacastQueueItem {
	char		Message[Settings.WiFi.UDPBroadcastQueue.MaxMessageSize + 1];
	uint32_t	Updated;
};

class WebServer_t {
	public:
		class Response {
			public:
				enum CODE	{ OK, INVALID, ERROR };
				enum TYPE 	{ PLAIN, JSON, HTML };

				string 		Body;
				CODE 		ResponseCode;
				TYPE		ContentType;

				Response();

				void 		SetSuccess();
				void 		SetFail();
				void		SetInvalid();

				void 		Clear();
		};

		WebServer_t();
		void Start();
		void Stop();

	    static esp_err_t GETHandler		(httpd_req_t *);
	    static esp_err_t POSTHandler	(httpd_req_t *);
	    static esp_err_t DELETEHandler	(httpd_req_t *);

	    static void	SendHTTPData(WebServer_t::Response& Response, httpd_req_t *Request);

		void UDPSendBroadcastAlive();
		void UDPSendBroadcastDiscover();
		void UDPSendBroadcastUpdated(uint8_t SensorID, string Value, uint8_t Repeat = 1);
		void UDPSendBroadcast(string);

		void UDPSendBroacastFromQueue();
		void UDPSendBroadcastQueueAdd(string Message);

		static string 		SetupPage;
		string GetSetupPage();

		static bool 		UDPServerStopFlag;
		static TaskHandle_t	UDPListenerTaskHandle;

		static void 	SetHeaders(WebServer_t::Response &, httpd_req_t *);

	private:
	    static QueueHandle_t UDPBroadcastQueue;
	    static httpd_handle_t HTTPServerHandle;

		static string	UDPAliveBody();
		static string	UDPDiscoverBody(string ID = "");
		static string	UDPUpdatedBody(uint8_t SensorID, string Value);

		static void		UDPListenerTask(void *);
};

#endif
