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
				enum TYPE 	{ PLAIN, JSON };

				string 		Body;
				CODE 		ResponseCode;
				TYPE		ContentType;

				Response();
				string 		toString();

				void 		SetSuccess();
				void 		SetFail();
				void		SetInvalid();

				void 		Clear();
			private:
				string ResponseCodeToString();
				string ContentTypeToString();
		};

		WebServer_t();
		void Start();
		void Stop();

		void UDPSendBroadcastAlive();
		void UDPSendBroadcastDiscover();
		void UDPSendBroadcastUpdated(uint8_t SensorID, string Value, uint8_t Repeat = 1);
		void UDPSendBroadcast(string);

		void UDPSendBroacastFromQueue();
		void UDPSendBroadcastQueueAdd(string Message);

	private:
	    static QueueHandle_t UDPBroadcastQueue;

		TaskHandle_t HTTPListenerTaskHandle;
		TaskHandle_t UDPListenerTaskHandle;

		static string UDPAliveBody();
		static string UDPDiscoverBody(string ID = "");
		static string UDPUpdatedBody(uint8_t SensorID, string Value);

		static void UDPListenerTask(void *);

		static void HTTPListenerTask(void *);
		static void HandleHTTP(struct netconn *);
		static void Write(struct netconn *conn, string Data);
};

#endif
