using namespace std;

#include <stdio.h>
#include <esp_log.h>

#include "include/Globals.h"
#include "include/WebServer.h"
#include "include/Query.h"
#include "include/Switch_str.h"
#include "include/API.h"

#include "drivers/FreeRTOS/FreeRTOS.h"

static char tag[] = "WebServer";

static WebServerTask_t WebServerTask = WebServerTask_t();

WebServer_t::WebServer_t() {
}

void WebServer_t::Start() {
  ESP_LOGD(tag, "Start");

  WebServerTask.setStackSize(8192);
	WebServerTask.start();
}

void WebServer_t::Stop() {
  ESP_LOGD(tag, "Stop");

  WebServerTask.stop();
}

void WebServerTask_t::run(void *data)
{
  ESP_LOGD(tag, "WebServerTask Run");

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

    WebServerResponse_t *Response = API::Handle(Query);
    string StrResponse = Response->toString();

    netconn_write(conn, StrResponse.c_str(), StrResponse.length(), NETCONN_NOCOPY);
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
  Result += "Content-type:" + ContentTypeToString() + "; charset=utf-8\r\n\r\n";
  Result += Body;

  return Result;
}

void WebServerResponse_t::SetSuccess() {
  ResponseCode  = CODE::OK;
  ContentType   = TYPE::JSON;
  Body          = "{\"success\" : \"true\"}";
}

void WebServerResponse_t::SetFail() {
  ResponseCode  = CODE::ERROR;
  ContentType   = TYPE::JSON;
  Body          = "{\"success\" : \"false\"}";
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
