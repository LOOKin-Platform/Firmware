/*
*    HTTPClient.h
*    Class to make HTTP Query
*
*/

#ifndef LIBS_HTTPCLIENT_H_
#define LIBS_HTTPCLIENT_H_

using namespace std;

#include "FreeRTOSWrapper.h"

#include <string>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>

#include <sys/socket.h>
#include <netdb.h>
#include <esp_log.h>

#include "Converter.h"
#include "Settings.h"

#include "esp_http_client.h"

enum QueryType { NONE, POST, GET, DELETE, PUT, PATCH };

typedef void (*ReadStarted)	(const char *);
typedef bool (*ReadBody)	(char[], int, const char *);
typedef void (*ReadFinished)(const char *);
typedef void (*Aborted)		(const char *);

/**
 * @brief Interface to performing HTTP queries
 */
class HTTPClient {
	public:

	/**
	 * @brief Struct to hold HTTP query data
	 */
		struct HTTPClientData_t {
			string      		URL			= "";				/*!< Hostname string, e. g. look-in.club */
			QueryType			Method		= QueryType::GET;	/*!< Query method, e. g. QueryType::POST */
			int 				BufferSize	= 0;
			string 				POSTData	= "";

			esp_http_client_handle_t
								Handle		= NULL;				/*!< Client handle */

			ReadStarted   		ReadStartedCallback   	= NULL;   /*!< Callback function invoked while started to read query data from server */
			ReadBody      		ReadBodyCallback      	= NULL;   /*!< Callback function invoked while query data reading process */
			ReadFinished  		ReadFinishedCallback  	= NULL;   /*!< Callback function invoked when query reading process is over */
			Aborted       		AbortedCallback       	= NULL;   /*!< Callback function invoked when reading data from server failed */
		};


		static void 			Query(HTTPClientData_t, bool ToFront = false, bool IsSystem = false);
		static void 			Query(string URL, QueryType Type = GET, bool ToFront = false, bool IsSystem = false,
									ReadStarted = NULL, ReadBody = NULL, ReadFinished = NULL, Aborted = NULL, string POSTData = "");

		static esp_err_t		QueryHandler(esp_http_client_event_t *event);
		static void				HTTPClientTask(void *);

		//static void 			RemoveFromQueueURLStartedWith(string URLStart, bool IsSystemQueue);

		static void				Failed              (HTTPClientData_t &);

		static void				CheckUserAgent		();

	private:
		static string			UserAgent;
		static QueueHandle_t  	Queue;

		static QueueHandle_t  	SystemQueue;
		static esp_http_client_handle_t SystemQueueHandler;

		static uint8_t        	ThreadsCounter;

		static map<uint32_t, HTTPClientData_t> QueryData;
};

#endif /* DRIVERS_HTTPCLIENT_H_ */
