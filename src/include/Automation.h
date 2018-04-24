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

		void			SensorChanged(uint8_t SensorID);
		static void	TimeChangedTask(void *);

		uint8_t		ScenarioCacheItemCount();
		void			AddScenarioCacheItem    	(Scenario_t);
		void			RemoveScenarioCacheItem 	(uint32_t);

		void			LoadVersionMap();
		bool			SetVersionMap(string SSID, uint32_t Version);
		string		SerializeVersionMap();

		void			HandleHTTPRequest(WebServer_t::Response &, QueryType Type, vector<string>, map<string,string>, string = "");

	private:
		TaskHandle_t TimeChangedHandle;

		vector<ScenarioCacheItem_t> ScenariosCache;
		map<string, uint32_t>       VersionMap;

		void  		Debug(Scenario_t);
		void  		Debug(ScenarioCacheItem_t);
};

#endif //AUTOMATION_H
