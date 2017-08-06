/*
*    HTTPClient.h
*    Class to make HTTP Query
*
*/

#ifndef DRIVERS_HTTPCLIENT_H_
#define DRIVERS_HTTPCLIENT_H_

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <netdb.h>
#include <esp_log.h>

#include "drivers/FreeRTOS/FreeRTOS.h"
#include "Task/Task.h"

#include "Settings.h"

using namespace std;

enum QueryType { NONE, POST, GET, DELETE };

typedef void (*ReadStarted)(char[]);
typedef bool (*ReadBody)(char[], int, char[]);
typedef bool (*ReadFinished)(char []);
typedef void (*Aborted)(char[]);

struct HTTPClientData_t {
  char      IP[15]        = "\0";
  uint8_t   Port          = 80;
  char      Hostname[32]  = "\0";
  char      ContentURI[32]= "\0";
  QueryType Method        = GET;

  int       SocketID  = -1;

  ReadStarted   ReadStartedCallback   = NULL;
  ReadBody      ReadBodyCallback      = NULL;
  ReadFinished  ReadFinishedCallback  = NULL;
  Aborted       AbortedCallback       = NULL;
};

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
