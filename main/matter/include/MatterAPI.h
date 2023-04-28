/*
*    HomeKitHTTPHandler.h
*    Class to handle API /HomeKit
*
*/

#ifndef MATTER_HTTP_HANDLER_H
#define MATTER_HTTP_HANDLER_H

#include "Globals.h"

class MatterAPI_t {
	public:
		static void	HandleHTTPRequest(WebServer_t::Response &, Query_t &);
};

#endif
