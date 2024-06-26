/*
*    Automation.h
*    API /Automation
*
*/

#ifndef AUTOMATION_H
#define AUTOMATION_H

#include <string>
#include <vector>

#include "Scenarios.h"
#include "Query.h"

#include "JSON.h"
#include "Converter.h"
#include "WiFi.h"
#include "Memory.h"

using namespace std;

#define  NVSScenariosArray        "Scenarios"
#define  NVSAutomationVersionMap  "VersionMap"

class Automation_t {
	public:
		struct ScenarioCacheItem_t {
			bool      IsLinked      = false;
			uint32_t  ScenarioID    = 0;
			uint8_t   ScenarioType  = 0;
			uint64_t  Operand       = 0;
		};

		Automation_t();
		void Init();

		uint32_t		CurrentVersion();

		static void		SensorChanged(uint8_t SensorID);
		static void		TimeChangedPool();

		uint8_t			ScenarioCacheItemCount();
		void			AddScenarioCacheItem    	(Scenario_t);
		void			RemoveScenarioCacheItem 	(uint32_t);

		void			LoadVersionMap();
		bool			SetVersionMap(string SSID, uint32_t Version);
		string			SerializeVersionMap();

		void			HandleHTTPRequest(WebServer_t::Response &, Query_t &);
		JSON 			RootInfo();

	private:
		static inline vector<ScenarioCacheItem_t> ScenariosCache 	= vector<ScenarioCacheItem_t>();
		static inline map<string, uint32_t>       VersionMap		= map<string,uint32_t>();

		static void  	Debug(Scenario_t);
		static void  	Debug(ScenarioCacheItem_t);
};

#endif //AUTOMATION_H
