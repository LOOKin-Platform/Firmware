#include "MatterAPI.h"
#include "Matter.h"
#include "BootAndRestore.h"

void MatterAPI_t::HandleHTTPRequest(WebServer_t::Response &Response, Query_t &Query)
{
	if (Query.GetURLPartsCount() == 2)
	{
		if (Query.Type == QueryType::POST)
		{
			if (Query.CheckURLPart("refresh", 1)) {
				Matter::AppServerRestart();
    			Response.SetSuccess();
    			return;
    		}

    		if (Query.CheckURLPart("reset", 1)) {
    			Matter::ResetData();
    			Response.SetSuccess();
    			return;
    		}

    		if (Query.CheckURLPart("reset-pairs", 1)) {
    			Matter::ResetPairs();
    			Response.SetSuccess();
    			return;
    		}

    		if (Query.CheckURLPart("mode", 1)) {
    			if (Settings.eFuse.Type != Settings.Devices.Remote) {
    				Response.SetFail();
    				return;
    			}

    			JSON JSONObject(Query.GetBody());

    			map<string,string> Values = JSONObject.GetItems();

    			if ((Values.count("token") == 0) ||
    				(Values.count("token") == 1 && Values["token"] != "mMLRHJMcrAHV")) {
    				Response.SetFail();
    				return;
    			}

    			uint8_t Mode = 0xFF;
    			if (Values.count("mode")) Mode = Converter::ToUint8(Values["mode"]);

    			if (Mode > 2) {
    				Response.SetFail();
    				return;
    			}

    			if ((Settings.eFuse.Model < 2 && Mode == 0x1) ||
    				(Settings.eFuse.Model >= 2 && Mode == 0x0))	{
    				Response.SetFail();
    				return;
    			}

    			Response.SetSuccess();

    			BootAndRestore::Reboot();

				return;
    		}
    	}
	}
}
