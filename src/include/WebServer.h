#ifndef WEBSERVER_H
#define WEBSERVER_H

using namespace std;

#include <string>

#include "../drivers/Task/Task.h"

extern "C" {
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
		static void Handle(struct netconn *conn);
};

class WebServerTask_t: public Task {
	void run(void *data) override;
};

class WebServerResponse_t
{
	public:
		enum CODE		{ OK, INVALID, ERROR };
		enum TYPE 	{ PLAIN, JSON };

    string 		Body;
		CODE 			ResponseCode;
		TYPE			ContentType;

		WebServerResponse_t();
		string 	toString();

		void 		SetSuccess();
		void 		SetFail();

	private:
		string ResponseCodeToString();
		string ContentTypeToString();
};

#endif
