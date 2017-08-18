/*
*    Webserver.cpp
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

#include "drivers/FreeRTOS/FreeRTOS.h"

class WebServer_t {
  public:
    WebServer_t();
    void Start();
    void Stop();

		void UDPSendBroadcastAlive();
    void UDPSendBroadcastDiscover();
    void UDPSendBroadcastUpdated(uint8_t SensorID, string Value);
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

class WebServerResponse_t
{
	public:
		enum CODE		{ OK, INVALID, ERROR };
		enum TYPE 	{ PLAIN, JSON };

    string 		Body;
		CODE 			ResponseCode;
		TYPE			ContentType;

		WebServerResponse_t();
		string 	toString();

		void 		SetSuccess();
		void 		SetFail();

	private:
		string ResponseCodeToString();
		string ContentTypeToString();
};

#endif
