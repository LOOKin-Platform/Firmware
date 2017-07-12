using namespace std;

#include <stdio.h>
#include <esp_log.h>

#include "include/Globals.h"
#include "include/WebServer.h"
#include "include/Query.h"
#include "include/Switch_str.h"
#include "include/API.h"

#include "drivers/FreeRTOS/FreeRTOS.h"

#define LED_BUILTIN GPIO_SEL_16

static char tag[] = "WebServer";

static WebServerTask_t WebServerTask = WebServerTask_t();

WebServer_t::WebServer_t() {
}

void WebServer_t::Start() {
  ESP_LOGD(tag, "WebServer::Start");

  WebServerTask.setStackSize(8192);
	WebServerTask.start();
}

void WebServer_t::Stop() {
  WebServerTask.stop();
}

void WebServerTask_t::run(void *data)
{
  ESP_LOGD(tag, "WebServerTask::run");

  struct netconn *conn, *newconn;
  err_t err;

  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 80);
  netconn_listen(conn);

  do
  {
    err = netconn_accept(conn, &newconn);

    if (err == ERR_OK)
    {
      WebServer_t::Handle(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);

  netconn_close(conn);
  netconn_delete(conn);
}

void WebServer_t::Handle(struct netconn *conn)
{
  ESP_LOGD(tag, "WebServer::http_server_netconn_serve");

  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;

  err = netconn_recv(conn, &inbuf);

  if (err == ERR_OK)
  {
	  netbuf_data(inbuf, (void * *)&buf, &buflen);

	  Query_t Query(buf);

    SWITCH(Query.RequestedUrl) {
      CASE("/on"):
        ESP_LOGD(tag, "GET Command: /on");
        break;
      CASE("/off"):
        ESP_LOGD(tag, "GET Command: /off");
        break;
      CASE("/status"):
        ESP_LOGD(tag, "GET Command: /status");
        break;
      DEFAULT:
        WebServerResponse_t *Response = API::Handle(Query);
        string StrResponse = Response->toString();

        netconn_write(conn, StrResponse.c_str(), StrResponse.length(), NETCONN_NOCOPY);
        break;
      }
  }
  netconn_close(conn);
  netbuf_delete(inbuf);
}

WebServerResponse_t::WebServerResponse_t() {
  ResponseCode = CODE::OK;
  ContentType  = TYPE::JSON;
}

string WebServerResponse_t::toString() {

  string Result = "";

  Result = "HTTP/1.1 " + ResponseCodeToString() + "\r\n";
  Result += "Content-type:" + ContentTypeToString() + "\r\n\r\n";
  Result += Body;

  return Result;
}

string WebServerResponse_t::ResponseCodeToString()
{
  switch (ResponseCode)
  {
    case CODE::INVALID  : return "400"; break;
    case CODE::ERROR    : return "500"; break;
    default             : return "200"; break;
  }
}

string WebServerResponse_t::ContentTypeToString()
{
  switch (ContentType)
  {
    case TYPE::PLAIN  : return "text/plain"; break;
    case TYPE::JSON   : return "application/json"; break;
    default           : return "";
  }
}
