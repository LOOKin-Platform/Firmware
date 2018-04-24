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

class WebServer_t {
	public:
		class Response {
			public:
				enum CODE	{ OK, INVALID, ERROR };
				enum TYPE 	{ PLAIN, JSON };

				string 		Body;
				CODE 		ResponseCode;
				TYPE			ContentType;

				Response();
				string 		toString();

				void 		SetSuccess();
				void 		SetFail();
				void			SetInvalid();
			private:
				string ResponseCodeToString();
				string ContentTypeToString();
		};

		WebServer_t();
		void Start();
		void Stop();

		void UDPSendBroadcastAlive();
		void UDPSendBroadcastDiscover();
		void UDPSendBroadcastUpdated(uint8_t SensorID, string Value, uint8_t Repeat = 3);
		void UDPSendBroadcast(string);

	private:
		TaskHandle_t HTTPListenerTaskHandle;
		TaskHandle_t UDPListenerTaskHandle;

		static string UDPAliveBody();
		static string UDPDiscoverBody(string ID = "");
		static string UDPUpdatedBody(uint8_t SensorID, string Value);

		static void UDPListenerTask(void *);

		static void HTTPListenerTask(void *);
		static void HandleHTTP(struct netconn *);
};

#endif
