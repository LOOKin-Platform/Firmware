/*
*    Automation.cpp
*    API /Automation
*
*/

#include "Automation.h"
#include "Globals.h"

const char tag[] 				= "Automation";
const char NVSAutomationArea[] 	= "Automation";

Automation_t::Automation_t() {
	ScenariosCache  = vector<ScenarioCacheItem_t>();
	VersionMap      = map<string, uint32_t>();
}

void Automation_t::Init() {
	ESP_LOGD(tag, "Init");

	Scenario_t::LoadScenarios();
	LoadVersionMap();
}

uint32_t Automation_t::CurrentVersion() {
	string SSID = WiFi_t::GetStaSSID();

	if (VersionMap.count(SSID) > 0)
		return VersionMap[SSID];

	return 0;
}

void Automation_t::SensorChanged(uint8_t SensorID) {
	ESP_LOGI(tag, "Sensor Changed. ID: %s", Converter::ToHexString(SensorID, 2).c_str());

	for (auto& ScenarioCacheItem : ScenariosCache)
		if (ScenarioCacheItem.IsLinked == true) {
			Scenario_t Scenario(ScenarioCacheItem.ScenarioType);
			Scenario.ID = ScenarioCacheItem.ScenarioID;

			if (!Scenario.IsEmpty()) {
				Scenario.SetData(ScenarioCacheItem.Operand);

				if (Scenario.Data->SensorUpdatedIsTriggered(SensorID))
					Scenario.Data->ExecuteCommands(ScenarioCacheItem.ScenarioID);
			}
	}
}

void Automation_t::TimeChangedPool() {
	for (auto& ScenarioCacheItem : Automation.ScenariosCache)
		if (ScenarioCacheItem.IsLinked == true && ScenarioCacheItem.ScenarioType == 0x02) {
			Scenario_t *Scenario = new Scenario_t(ScenarioCacheItem.ScenarioType);
			Scenario->SetData(ScenarioCacheItem.Operand);

			if (Scenario->Data->TimeUpdatedIsTriggered())
				Scenario->Data->ExecuteCommands(ScenarioCacheItem.ScenarioID);

			delete Scenario;
		}
}

void Automation_t::HandleHTTPRequest(WebServer_t::Response &Result, Query_t &Query) {
	// обработка GET запроса - получение данных
	if (Query.Type == QueryType::GET) {
		if (Query.GetURLPartsCount() == 1) {
			Result.Body = RootInfo().ToString();
			return;
		}

		// Запрос конкретного параметра или команды секции API
		if (Query.GetURLPartsCount() == 2) {
			if (Query.CheckURLPart("version"	,1))	Result.Body = Converter::ToHexString(CurrentVersion(), 8);
			if (Query.CheckURLPart("versionmap"	,1))	Result.Body = SerializeVersionMap();
			if (Query.CheckURLPart("scenarios"	,1))
			{
				vector<string> Scenarios = vector<string>();

				for (auto& Scenario : ScenariosCache)
					Scenarios.push_back(Converter::ToHexString(Scenario.ScenarioID, 8));

				Result.Body = JSON::CreateStringFromVector(Scenarios);
			}
		}

		if (Query.GetURLPartsCount() == 3) {
			if (Query.CheckURLPart("scenarios"	,1)) {
				Scenario_t Scenario;

				Scenario_t::LoadScenario(Scenario, Converter::UintFromHexString<uint32_t>(Query.GetStringURLPartByNumber(2)));

				if (!Scenario.IsEmpty())
					Result.Body = Scenario_t::SerializeScene(Scenario);
				else {
					Result.ResponseCode = WebServer_t::Response::CODE::INVALID;
					Result.Body = "{\"success\" : \"false\", \"message\" : \"Scenario doesn't exist\"}";
				}
			}
		}
	}

	// POST запрос - сохранение и изменение данных
	if (Query.Type == QueryType::POST) {
		if (Query.GetURLPartsCount() == 1) {
			map<string,string> Params = JSON(Query.GetBody()).GetItems();

			if (Params.count("version") > 0) {
				string SSID = (Params.count("SSID") > 0) ? Params["SSID"] : WiFi_t::GetStaSSID();

				if (SetVersionMap(SSID, Converter::UintFromHexString<uint32_t>(Params["version"])))
					Result.Body = "{\"success\" : \"true\"}";
				else {
					Result.ResponseCode = WebServer_t::Response::CODE::INVALID;
					Result.Body = "{\"success\" : \"false\", \"message\" : \"Error while version processing\"}";
				}
			}
		}

		if (Query.GetURLPartsCount() == 2) {
			if (Query.CheckURLPart("scenarios", 1)) {
				Scenario_t Scenario = Scenario_t::DeserializeScene(Query.GetBody());
				bool IsFound = false;

				if (!Scenario.IsEmpty()) {
					for (int i=0; i < ScenariosCache.size(); i++)
						if (ScenariosCache[i].ScenarioID == Scenario.ID) {
							IsFound = true;
							break;
						}

					if (!IsFound) {
						if (Scenario_t::SaveScenario(Scenario)) {
							Result.Body = "{\"success\" : \"true\"}";
						}
						else {
							Result.ResponseCode = WebServer_t::Response::CODE::ERROR;
							Result.Body = "{\"success\" : \"false\", \"message\" : \"Error while scenario insertion\"}";
						}
					}
					else {
						Result.ResponseCode = WebServer_t::Response::CODE::INVALID;
						Result.Body = "{\"success\" : \"false\", \"message\" : \"Scenario already set\"}";
					}
				}
			}
		}
	}

	// DELETE запрос - удаление сценариев
	if (Query.Type == QueryType::DELETE) {
		if (Query.GetURLPartsCount() == 1) {
			if (Settings.DeviceGeneration == 1)
				SPIFlash::EraseRange(Settings.Scenarios.Memory.Start4MB, Settings.Scenarios.Memory.Size4MB);
			else
			{
				SPIFlash::EraseRange(Settings.Scenarios.Memory.Start16MB, Settings.Scenarios.Memory.Size16MB);
				/*
				 * idfupdate - replace for this after ESP_PARTITION_TYPE_ANY will be avaliable

				PartitionAPI::ErasePartition("scenarios");
				*/
			}

			ScenariosCache.clear();
			VersionMap.clear();

			NVS Memory(NVSAutomationArea);
			Memory.SetString(NVSAutomationVersionMap, "");

			Result.ResponseCode = WebServer_t::Response::CODE::OK;
			Result.Body = "{\"success\" : \"true\"}";
		}

		if (Query.GetURLPartsCount() == 3) {
			if (Query.CheckURLPart("scenarios",1)) {
				vector<string> ScenariosToDelete = Converter::StringToVector(Query.GetStringURLPartByNumber(2), ",");

				for (auto& ScenarioToDelete : ScenariosToDelete) {
					uint32_t ScenarioID = Converter::UintFromHexString<uint32_t>(ScenarioToDelete);
					Scenario_t::RemoveScenario(ScenarioID);
					RemoveScenarioCacheItem(ScenarioID);
				}

				if (ScenariosToDelete.size() > 0) {
					Result.ResponseCode = WebServer_t::Response::CODE::OK;
					Result.Body = "{\"success\" : \"true\"}";
				}
				else {
					Result.ResponseCode = WebServer_t::Response::CODE::INVALID;
					Result.Body = "{\"success\" : \"false\", \"message\":\"empty scenario ID\"}";
				}
			}
		}
	}
}

JSON Automation_t::RootInfo() {
	JSON JSONObject;
	JSONObject.SetItem("Version", Converter::ToHexString(CurrentVersion(), 8));

	vector<map<string,string>> tmpVector = vector<map<string,string>>();
	for (map<string, uint32_t>::iterator it = VersionMap.begin(); it != VersionMap.end(); ++it) {
		tmpVector.push_back({
			{ "SSID"  	, it->first},
			{ "Version"	, Converter::ToHexString(it->second, 8)}
		});
	}

	JSONObject.SetObjectsArray("VersionMap", tmpVector);

	vector<string> SceneIDs;
	for (auto& Scenario : ScenariosCache)
		SceneIDs.push_back(Converter::ToHexString(Scenario.ScenarioID, 8));

	JSONObject.SetStringArray("Scenarios", SceneIDs);
	return JSONObject;
}


uint8_t Automation_t::ScenarioCacheItemCount() {
	return ScenariosCache.size();
}

void Automation_t::AddScenarioCacheItem(Scenario_t Scenario) {
	ScenarioCacheItem_t NewCacheItem;

	NewCacheItem.IsLinked       = Scenario.Data->IsLinked(Settings.eFuse.DeviceID, Scenario.Commands);
	NewCacheItem.ScenarioID     = Scenario.ID;
	NewCacheItem.ScenarioType   = Scenario.Type;
	NewCacheItem.Operand        = Converter::UintFromHexString<uint64_t>(Scenario.Data->ToString());

	ScenariosCache.push_back(NewCacheItem);
}

void Automation_t::RemoveScenarioCacheItem(uint32_t ScenarioID) {
  for (int i=0; i < ScenariosCache.size(); i++)
    if (ScenariosCache[i].ScenarioID == ScenarioID) {
      ScenariosCache.erase(ScenariosCache.begin() + i);
      return;
    }
}

void Automation_t::LoadVersionMap() {
	NVS Memory(NVSAutomationArea);

	JSON JSONObject(Memory.GetString(NVSAutomationVersionMap));

	vector<map<string,string>> Items = JSONObject.GetObjectsArray();

	for (int i=0; i < Items.size(); i++) {
		if (!Items[i]["ssid"].empty())
			VersionMap[Items[i]["ssid"]] = Converter::UintFromHexString<uint32_t>(Items[i]["version"]);
	}
}

bool Automation_t::SetVersionMap(string SSID, uint32_t Version) {
	if (SSID.empty())
		return false;

	VersionMap[SSID] = Version;

	NVS Memory(NVSAutomationArea);

	Memory.SetString(NVSAutomationVersionMap, SerializeVersionMap());
	Memory.Commit();

	return true;
}

string Automation_t::SerializeVersionMap() {
	JSON JSONObject;

	vector<map<string,string>> tmpVector = vector<map<string,string>>();
	for (map<string, uint32_t>::iterator it = VersionMap.begin(); it != VersionMap.end(); ++it) {
		tmpVector.push_back({
			{ "SSID"  	, it->first},
			{ "Version"	, Converter::ToHexString(it->second, 8)}
		});
	}

  JSONObject.SetObjectsArray("", tmpVector);

  return JSONObject.ToString();
}

void Automation_t::Debug(ScenarioCacheItem_t Item) {
	ESP_LOGI(tag, "IsLinked     = %u"   , Item.IsLinked);
	ESP_LOGI(tag, "ScenarioID   = %s"   , Converter::ToHexString(Item.ScenarioID, 2).c_str());
	ESP_LOGI(tag, "ScenarioType = %s"   , Converter::ToHexString(Item.ScenarioType,2).c_str());
	ESP_LOGI(tag, "Operand      = %s"   , Converter::ToHexString(Item.Operand, 16).c_str());
}

void Automation_t::Debug(Scenario_t Scene) {
	ESP_LOGI(tag, "Scene Type     = %s"   , Converter::ToHexString(Scene.Type,2).c_str());
	ESP_LOGI(tag, "Scene ID       = %s"   , Converter::ToHexString(Scene.ID,8).c_str());
	//ESP_LOGI(tag, "Scene Name   = %s"   , Scene.Name.c_str());
	ESP_LOGI(tag, "Scene Operand  = %s"   , Scene.GetDataHexString().c_str());

	for (int i=0; i < Scene.Commands.size(); i++) {
		ESP_LOGI(tag, "Command[%u] DeviceID   = %s", i, Converter::ToHexString(Scene.Commands[i].DeviceID,8).c_str());
		ESP_LOGI(tag, "Command[%u] Command    = %s", i, Converter::ToHexString(Scene.Commands[i].CommandID,2).c_str());
		ESP_LOGI(tag, "Command[%u] Event      = %s", i, Converter::ToHexString(Scene.Commands[i].EventCode,2).c_str());
	}
}
