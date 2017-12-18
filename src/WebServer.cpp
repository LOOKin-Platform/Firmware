/*
*    Webserver.cpp
*    Class for handling TCP and UDP connections
*
*/

using namespace std;

#include <stdio.h>
#include <string.h>

#include "Globals.h"
#include "WebServer.h"

#include <FreeRTOS/FreeRTOS.h>

#include "Query.h"
#include "API.h"

#include <esp_log.h>

static char tag[] = "WebServer";

WebServer_t::WebServer_t() {
  HTTPListenerTaskHandle  = NULL;
  UDPListenerTaskHandle   = NULL;
}

void WebServer_t::Start() {
  ESP_LOGD(tag, "Start");

  HTTPListenerTaskHandle  = FreeRTOS::StartTask(HTTPListenerTask, "HTTPListenerTask", NULL, 8192);
  UDPListenerTaskHandle   = FreeRTOS::StartTask(UDPListenerTask, "UDPListenerTask"  , NULL, 4096);
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

void WebServer_t::UDPSendBroadcastUpdated(uint8_t SensorID, string Value) {
  UDPSendBroadcast(UDPUpdatedBody(SensorID, Value));
}

string WebServer_t::UDPAliveBody() {
  string Message = "Alive!" + Device->IDToString() + ":" + Device->Type->ToHexString() + ":"  + inet_ntoa(Network->IP);
  return UDP_PACKET_PREFIX + Message;
}

string WebServer_t::UDPDiscoverBody(string ID) {
  string Message = (ID != "") ? ("Discover!" + ID) : "Discover!";
  return UDP_PACKET_PREFIX + Message;
}

string WebServer_t::UDPUpdatedBody(uint8_t SensorID, string Value) {
  string Message = "Updated!" + Device->IDToString() + ":" + Converter::ToHexString(SensorID, 2) + ":" + Value;
  return UDP_PACKET_PREFIX + Message;
}

void WebServer_t::UDPSendBroadcast(string Message) {
  struct netconn *Connection;
  struct netbuf *Buffer;
  char *Data;

  Connection = netconn_new( NETCONN_UDP );
  netconn_connect(Connection, IP_ADDR_BROADCAST, UDP_SERVER_PORT );

  Buffer  = netbuf_new();
  Data    = (char *)netbuf_alloc(Buffer, Message.length());
  memcpy (Data, Message.c_str(), Message.length());
  netconn_send(Connection, Buffer);

  netbuf_delete(Buffer); // De-allocate packet buffer
  netconn_close(Connection);

  ESP_LOGI(tag, "UDP broadcast \"%s\" sended", Message.c_str());
}

void WebServer_t::UDPListenerTask(void *data)
{
  ESP_LOGD(tag, "UDPListenerTask Run");

  struct netconn *Connection;
  struct netbuf *inBuffer, *outBuffer;
  char *inData, *outData;
  u16_t inDataLen;
  err_t err;

  Connection = netconn_new(NETCONN_UDP);
  netconn_bind(Connection, IP_ADDR_ANY, UDP_SERVER_PORT);

  do {
    err = netconn_recv(Connection, &inBuffer);

    if (err == ERR_OK) {
      netbuf_data(inBuffer, (void * *)&inData, &inDataLen);
      string Datagram = inData;

      ESP_LOGI(tag, "UDP RECEIVED \"%s\"", Datagram.c_str());

      outBuffer = netbuf_new();

      // answer to the Discover query
      if (Datagram == WebServer_t::UDPDiscoverBody() || Datagram == WebServer_t::UDPDiscoverBody(Device->IDToString())) {
        string Answer = WebServer_t::UDPAliveBody();

        outData = (char *)netbuf_alloc(outBuffer, Answer.length());
        memcpy (outData, Answer.c_str(), Answer.length());

        netconn_sendto(Connection, outBuffer, &inBuffer->addr, inBuffer->port);

        ESP_LOGD(tag, "UDP \"%s\" sended to %s:%u", Answer.c_str(), inet_ntoa(inBuffer->addr), inBuffer->port);
      }

      string AliveText = UDP_PACKET_PREFIX + string("Alive!");
      size_t AliveFound = Datagram.find(AliveText);

      if (AliveFound != string::npos) {
        string Alive = Datagram;
        Alive.replace(Alive.find(AliveText),AliveText.length(),"");
        vector<string> Data = Converter::StringToVector(Alive, ":");

        if (Data.size() > 2)
          Network->DeviceInfoReceived(Data[0], Data[1], Data[2]);
      }


      netbuf_delete(outBuffer);
      netbuf_delete(inBuffer);
    }

  } while(err == ERR_OK);

  netbuf_delete(inBuffer);
  netconn_close(Connection);
  netconn_delete(Connection);
}

void WebServer_t::HTTPListenerTask(void *data) {
  ESP_LOGD(tag, "HTTPListenerTask Run");

  struct netconn *conn, *newconn;
  err_t err;

  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 80);
  netconn_listen(conn);

  do {
    err = netconn_accept(conn, &newconn);

    if (err == ERR_OK) {
      WebServer_t::HandleHTTP(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);

  netconn_close(conn);
  netconn_delete(conn);
}

void WebServer_t::HandleHTTP(struct netconn *conn) {
  ESP_LOGD(tag, "Handle HTTP Query");

  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;

  err_t err;
  string HTTPString = "";

  netconn_set_recvtimeout(conn, 50);    // timeout on 10 msecs

  while((err = netconn_recv(conn,&inbuf)) == ERR_OK) {
    do {
      netbuf_data(inbuf, (void * *)&buf, &buflen);
      HTTPString += string(buf);
    } while(netbuf_next(inbuf) >= 0);

    netbuf_delete(inbuf);
  }

  if (!HTTPString.empty()) {
    Query_t Query(HTTPString);
    WebServer_t::Response Result;

    API::Handle(Result, Query);
    netconn_write(conn, Result.toString().c_str(), Result.toString().length(), NETCONN_NOCOPY);
  }

  netconn_close(conn);
  netbuf_delete(inbuf);
}

/*
*    WebServer_t::Response
*    class for building user-friendly browser answer
*
*/

WebServer_t::Response::Response() {
  ResponseCode = CODE::OK;
  ContentType  = TYPE::JSON;
}

string WebServer_t::Response::toString() {
  string Result = "";

  Result = "HTTP/1.1 " + ResponseCodeToString() + "\r\n";
  Result += "Content-type:" + ContentTypeToString() + "; charset=utf-8\r\n\r\n";
  Result += Body;

  return Result;
}

void WebServer_t::Response::SetSuccess() {
  ResponseCode  = CODE::OK;
  ContentType   = TYPE::JSON;
  Body          = "{\"success\" : \"true\"}";
}

void WebServer_t::Response::SetFail() {
  ResponseCode  = CODE::ERROR;
  ContentType   = TYPE::JSON;
  Body          = "{\"success\" : \"false\"}";
}

string WebServer_t::Response::ResponseCodeToString() {
  switch (ResponseCode) {
    case CODE::INVALID  : return "400"; break;
    case CODE::ERROR    : return "500"; break;
    default             : return "200"; break;
  }
}

string WebServer_t::Response::ContentTypeToString() {
  switch (ContentType) {
    case TYPE::PLAIN  : return "text/plain"; break;
    case TYPE::JSON   : return "application/json"; break;
    default           : return "";
  }
}
