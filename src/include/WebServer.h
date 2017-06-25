#ifndef WEBSERVER_H
#define WEBSERVER_H

extern "C" {

	#include "freertos/FreeRTOS.h"

	#include "freertos/portmacro.h"

	#include "lwip/sys.h"
	#include "lwip/netdb.h"
	#include "lwip/api.h"
	#include "lwip/err.h"
}

class WebServer_t {
  public:
    WebServer_t();
    void Start();
    void Stop();

    static void http_server_netconn_serve(struct netconn *);
    static void http_server(void *);

  private:

    TaskHandle_t xHandle;
};

#endif
