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
#include "lwip/ip.h"

#include "esp_http_server.h"

#include "FreeRTOSWrapper.h"

#include "Settings.h"

struct UDPBroacastQueueItem {
	char		Message[Settings.WiFi.UDPBroadcastQueue.MaxMessageSize + 1];
	uint32_t	Updated;
};

class WebServer_t {
	public:
		enum QueryTransportType { WebServer = 0x0, MQTT = 0x1, Undefined = 0xFF };

		class Response {
			public:
				enum CODE		{ OK, INVALID, ERROR, IGNORE };
				enum TYPE 		{ PLAIN, JSON, HTML };

				string 			Body 			= "";
				CODE 			ResponseCode	= CODE::OK;
				TYPE			ContentType		= TYPE::JSON ;

				void 			SetSuccess();
				void 			SetFail(string Reason = "");
				void			SetInvalid(string Reason = "");

				void 			Clear();

				uint16_t		CodeToInt();
		};

		WebServer_t();
		void 					HTTPStart();
		void 					HTTPStop();

		void 					UDPStart();
		void 					UDPStop();

		void RegisterHandlers	(httpd_handle_t);
		void UnregisterHandlers	(httpd_handle_t);

	    static esp_err_t 		GETHandler		(httpd_req_t *);
	    static esp_err_t 		POSTHandler		(httpd_req_t *);
	    static esp_err_t 		PUTHandler		(httpd_req_t *);
	    static esp_err_t 		DELETEHandler	(httpd_req_t *);
	    static esp_err_t 		PATCHHandler	(httpd_req_t *);

	    static void				SendHTTPData(WebServer_t::Response& Response, httpd_req_t *Request, bool TerminateSession = true);

	    static void 			SendChunk(httpd_req_t *Request, string Part);
	    static void 			EndChunk(httpd_req_t *Request);

		void 					UDPSendBroadcastAlive();
		void 					UDPSendBroadcastDiscover();
		void 					UDPSendBroadcastUpdated(uint8_t SensorID, string Value, uint8_t Repeat = 1, string Operand = "");
		void 					UDPSendBroadcast(string, bool IsScheduled = true);

		void 					UDPSendBroacastFromQueue();
		void 					UDPSendBroadcastQueueAdd(string Message);

		static string 			SetupPage;
		string 					GetSetupPage();

		static bool 			UDPServerStopFlag;
		static TaskHandle_t		UDPListenerTaskHandle;

		static void 			SetHeaders(WebServer_t::Response &, httpd_req_t *);

		static string			UDPAliveBody();
		static string			UDPDiscoverBody(string ID = "");
		static string			UDPUpdatedBody(string SensorID, string Value, string Operand = "");

		static string			AllowOriginHeader;
	private:

	    static QueueHandle_t 	UDPBroadcastQueue;
	    static httpd_handle_t	HTTPServerHandle;


		static void				UDPListenerTask(void *);
};

#endif
