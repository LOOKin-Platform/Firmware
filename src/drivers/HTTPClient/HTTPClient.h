/*
*    HTTPClient.h
*    Class to make HTTP Query
*
*/

#ifndef DRIVERS_HTTPCLIENT_H_
#define DRIVERS_HTTPCLIENT_H_

#include "FreeRTOSWrapper.h"

#include <string>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <sys/socket.h>
#include <netdb.h>
#include <esp_log.h>

#include "../../include/Settings.h"

using namespace std;

enum QueryType { NONE, POST, GET, DELETE };

typedef void (*ReadStarted)(char[]);
typedef bool (*ReadBody)(char[], int, char[]);
typedef void (*ReadFinished)(char []);
typedef void (*Aborted)(char[]);

/**
 * @brief Struct to hold HTTP query data
 */
struct HTTPClientData_t {
  char      IP[15]        = "\0";               /*!< Server IP string, e. g. 192.168.1.1 */
  uint8_t   Port          = 80;                 /*!< Server port, e. g. 80 */
  char      Hostname[32]  = "\0";               /*!< Hostname string, e. g. look-in.club */
  char      ContentURI[32]= "\0";               /*!< Path to the content in the server, e. g. /api/v1/time */
  QueryType Method        = GET;                /*!< Query method, e. g. QueryType::POST */

  int       SocketID      = -1;                 /*!< Socket identifier */

  ReadStarted   ReadStartedCallback   = NULL;   /*!< Callback function invoked while started to read query data from server */
  ReadBody      ReadBodyCallback      = NULL;   /*!< Callback function invoked while query data reading process */
  ReadFinished  ReadFinishedCallback  = NULL;   /*!< Callback function invoked when query reading process is over */
  Aborted       AbortedCallback       = NULL;   /*!< Callback function invoked when reading data from server failed */
};

/**
 * @brief Interface to performing HTTP queries
 */
class HTTPClient {
  public:
    static void Query(HTTPClientData_t, bool = false);
    static void Query(string Hostname, uint8_t Port, string ContentURI, QueryType Type = GET, string IP="", bool ToFront = false,
            ReadStarted = NULL, ReadBody=NULL,ReadFinished=NULL, Aborted=NULL);

    static void HTTPClientTask(void *);

    static bool     Connect             (HTTPClientData_t &);
    static char*    ResolveIP           (const char *Hostname);
    static int      ReadUntil           (char *buffer, char delim, int len);
    static bool     ReadPastHttpHeader  (HTTPClientData_t &, char text[], int total_len);
    static void     Failed              (HTTPClientData_t &);

  private:
    static QueueHandle_t  Queue;
    static uint8_t        ThreadsCounter;
};

#endif /* DRIVERS_HTTPCLIENT_H_ */
