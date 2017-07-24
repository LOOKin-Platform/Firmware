using namespace std;

#include <stdio.h>
#include <string.h>

#include "drivers/FreeRTOS/FreeRTOS.h"

#include "Globals.h"
#include "WebServer.h"

#include "Query.h"
#include "Switch_str.h"
#include "API.h"

#include <esp_log.h>

static char tag[] = "WebServer";

/*
  WebServer_t - class for manage UDP and TCP connections
*/

WebServer_t::WebServer_t() {
  HTTPListenerTaskHandle = NULL;
}

void WebServer_t::Start() {
  ESP_LOGD(tag, "Start");

  HTTPListenerTaskHandle  = FreeRTOS::StartTask(HTTPListenerTask, "HTTPListenerTask", NULL, 8192);
  UDPListenerTaskHandle   = FreeRTOS::StartTask(UDPListenerTask, "UDPListenerTask"  , NULL, 2056);
}

void WebServer_t::Stop() {
  ESP_LOGD(tag, "Stop");

  FreeRTOS::DeleteTask(HTTPListenerTaskHandle);
  FreeRTOS::DeleteTask(UDPListenerTaskHandle);
}

void WebServer_t::UDPSendBroadcastAlive() {
  UDPSendBroadcast(UDPAliveBody());
}

void WebServer_t::UDPSendBroadcastDiscover() {
  UDPSendBroadcast(UDPDiscoverBody());
}

string WebServer_t::UDPAliveBody() {
  return "Alive!" + Device->ID + ":" + inet_ntoa(Network->IP);
}

string WebServer_t::UDPDiscoverBody(string ID) {
  return "Discover!" + (ID != "") ? ID : "";
}

void WebServer_t::UDPSendBroadcast(string Datagram) {
  struct netconn *Connection;
  struct netbuf *Buffer;
  char *Data;

  string Message = UDP_PACKET_PREFIX + Datagram;

  Connection = netconn_new( NETCONN_UDP );
  netconn_connect(Connection, IP_ADDR_BROADCAST, UDP_SERVER_PORT );

  Buffer  = netbuf_new();
  Data    = (char *)netbuf_alloc(Buffer, Message.length());
  memcpy (Data, Message.c_str(), Message.length());
  netconn_send(Connection, Buffer);

  netbuf_delete(Buffer); // De-allocate packet buffer

  ESP_LOGI(tag, "UDP broadcast %s sended", Message.c_str());
}

void WebServer_t::UDPListenerTask(void *data)
{
  ESP_LOGD(tag, "UDPListenerTask Run");

  struct netconn *Connection;
  struct netbuf *Buffer;
  char *Data;
  u16_t DataLen;

  err_t err;

  Connection = netconn_new(NETCONN_UDP);
  netconn_bind(Connection, IP_ADDR_ANY, UDP_SERVER_PORT);

  do {
    err = netconn_recv(Connection, &Buffer);

    if (err == ERR_OK)
    {
      netbuf_data(Buffer, (void * *)&Data, &DataLen);
      ESP_LOGI(tag, "UDP RECEIVED %s", Data);

      string Datagram = Data;

      if (Data == WebServer_t::UDPDiscoverBody() || Data == WebServer_t::UDPDiscoverBody(Device->ID))
      {
        string Answer = WebServer_t::UDPAliveBody();

        struct netconn *AnswerConnection = netconn_new(NETCONN_UDP);
        struct netbuf *AnswerBuffer = netbuf_new();

        netconn_bind(AnswerConnection, &Buffer->addr, UDP_SERVER_PORT);

        char *Data = (char *)netbuf_alloc(AnswerBuffer, Answer.length());
        memcpy (Data, Answer.c_str(), Answer.length());
        netconn_send(AnswerConnection, AnswerBuffer);

        netbuf_delete(AnswerBuffer);
        netconn_close(AnswerConnection);
        netconn_delete(AnswerConnection);
      }
    }

  } while(err == ERR_OK);

  netbuf_delete(Buffer);
  netconn_close(Connection);
  netconn_delete(Connection);
}

void WebServer_t::HTTPListenerTask(void *data)
{
  ESP_LOGD(tag, "HTTPListenerTask Run");

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
      WebServer_t::HandleHTTP(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);

  netconn_close(conn);
  netconn_delete(conn);
}

void WebServer_t::HandleHTTP(struct netconn *conn)
{
  ESP_LOGD(tag, "WebServer HandleHTTP");

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

/*
  WebServerResponse_t - class for building user-friendly browser answer
*/

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
