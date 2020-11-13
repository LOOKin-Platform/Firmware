/*
*    HomeKitHTTPHandler.h
*    Class to handle API /HomeKit
*
*/

#ifndef HOMEKIT_HTTP_HANDLER_H
#define HOMEKIT_HTTP_HANDLER_H

#include "Globals.h"
#include "FreeRTOSTask.h"

class HomeKitAPI_t {
	public:
		static void	HandleHTTPRequest(WebServer_t::Response &, Query_t &);
};

class TestTask : public Task {
	void Run(void *);
};

#endif
