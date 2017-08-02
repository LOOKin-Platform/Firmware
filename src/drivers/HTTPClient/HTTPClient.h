#ifndef DRIVERS_HTTPCLIENT_H_
#define DRIVERS_HTTPCLIENT_H_

using namespace std;

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <esp_log.h>

#include "Task/Task.h"

class HTTPClientTask_t : public Task {
  public:
    void  Run(void *data) override;
};

class HTTPClient_t {
  friend class HTTPClientTask_t;
  public:
    string IP;
    string Port;
    string Hostname;
    string ContentURI;
    string Method;

    HTTPClient_t();
    virtual ~HTTPClient_t() = default;

    void  Request();

    virtual void ReadStarted()					{};
    virtual bool ReadBody(char [], int)	{ return true; };
    virtual bool ReadFinished()					{ return true; };

    virtual void Aborted()              {};
  private:
    int   BufferSize;
    int   SocketID;
    char  HTTPRequest[128];

    HTTPClientTask_t  *Task;

    bool  HttpConnect();
    int   ReadUntil(char *buffer, char delim, int len);
    bool  ReadPastHttpHeader(char text[], int total_len);
    void  __attribute__((noreturn)) task_fatal_error();
};

#endif /* DRIVERS_HTTPCLIENT_H_ */
