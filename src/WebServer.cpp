using namespace std;

#include <stdio.h>

#include "include/WebServer.h"
#include "include/Query.h"
#include "include/Switch_str.h"
#include "include/API.h"

#define LED_BUILTIN GPIO_SEL_16

WebServer_t::WebServer_t() {
  xHandle = NULL;
}

void WebServer_t::Start() {
  xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &xHandle);
}

void WebServer_t::Stop() {
  vTaskDelete(xHandle);
}

void WebServer_t::http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);

  if (err == ERR_OK)
  {
	  netbuf_data(inbuf, (void * *)&buf, &buflen);

	  Query_t Query(buf);

    if (Query.Type == GET) {
        SWITCH(Query.RequestedUrl) {
          CASE("/on"):
            printf("GET Command: /on");
            break;
          CASE("/off"):
            printf("GET Command: /off");
            break;
          CASE("/status"):
            printf("GET Command: /status");
            break;
          DEFAULT:
            string Response = API::Handle(Query);
            netconn_write(conn, Response.c_str(), sizeof(Response.c_str())-1, NETCONN_NOCOPY);
            break;
        }
    }
  }
  netconn_close(conn);
  netbuf_delete(inbuf);
}

void WebServer_t::http_server(void *pvParameters)
{
  struct netconn *conn, *newconn;
  err_t err;

  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 80);

  netconn_listen(conn);
  do {
     err = netconn_accept(conn, &newconn);
     if (err == ERR_OK) {
       http_server_netconn_serve(newconn);
       netconn_delete(newconn);
     }
   } while(err == ERR_OK);
   netconn_close(conn);
   netconn_delete(conn);
}
