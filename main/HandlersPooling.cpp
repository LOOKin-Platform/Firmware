#include "HandlersPooling.h"

void HandlersPooling_t::BARCheck() {
	EnergyPeriodicHandler::InnerHandler(true);
}

uint8_t HandlersPooling_t::RegisterHandler(PoolingHandler_fn Handler, uint32_t PeriodU, uint32_t Priority) 
{
	uint8_t Index = 0;

	// Finding new handler ID
	std::vector<uint8_t> HandlerIDs = vector<uint8_t>();
	for (auto& HandlerItem : Handlers)
		HandlerIDs.push_back(HandlerItem.Index);

	while (std::find(HandlerIDs.begin(), HandlerIDs.end(), Index) != HandlerIDs.end())
		Index++;

	ESP_LOGE("Handler Index", "%d", Index);

	HandlerItem_t NewHandler;
	NewHandler.Index 	= Index;
	NewHandler.Handler 	= Handler;
	NewHandler.PeriodU 	= PeriodU * 1000;
	NewHandler.Priority = Priority;

	Handlers.push_back(NewHandler);

	return Index;
}


void HandlersPooling_t::CheckHandlers() {
	uint64_t UptimeU = Time::UptimeU();

	if (LastHandlerFired == 0) {
		LastHandlerFired = UptimeU;
		return;
	}

	uint32_t Delta = UptimeU - LastHandlerFired;

	for (HandlerItem_t& HandlerItem : Handlers) {
		HandlerItem.PeriodTimer += Delta;

		if (HandlerItem.PeriodTimer >= HandlerItem.PeriodU)
		{	
			//DELAY = HandlerItem.PeriodTimer - HandlerItem.PeriodU
			HandlerItem.Handler();
			HandlerItem.PeriodTimer = 0;
		}
	}

	LastHandlerFired = UptimeU;
}

void HandlersPooling_t::Pool () {
	WiFiUptimeHandler::Start();

	RegisterHandler(&HandlersPooling_t::OverheatHandler::Pool				, 900);
	RegisterHandler(&HandlersPooling_t::WiFiUptimeHandler::Pool				, 950);
	RegisterHandler(&HandlersPooling_t::EnergyPeriodicHandler::Pool			, 500);
	RegisterHandler(&HandlersPooling_t::WirelessPriorityHandler::Pool		, 490);
	RegisterHandler(&HandlersPooling_t::SensorPeriodicHandler::Pool			, 500);
	RegisterHandler(&HandlersPooling_t::RemoteControlPeriodicHandler::Pool	, 500);
	RegisterHandler(&HandlersPooling_t::PingPeriodicHandler::Pool			, 1000); // Once a second
	RegisterHandler(&HandlersPooling_t::NetworkMapHandler::Pool				, 2000); // Once a 2 seconds

	RegisterHandler(&Automation_t::TimeChangedPool							, 1000); // Once a second
	RegisterHandler(&Scenario_t::ExecuteCommandsPool						, 76); 
	
	// Bluetooth handler switched off

	while (1) 
	{
		if (Time::Uptime() % 10 == 0)
			ESP_LOGI("Pooling","RAM left %d", esp_get_free_heap_size());

		CheckHandlers();

		FreeRTOS::Sleep(Settings.Pooling.Interval);

		/*
		if (Time::Uptime() % 100 == 0)
			FreeRTOS::GetRunTimeStats();
		*/
	}
}
