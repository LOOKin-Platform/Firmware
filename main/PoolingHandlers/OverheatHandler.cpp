#include "HandlersPooling.h"

#include "Log.h"
#include "FreeRTOSWrapper.h"

//!extern "C" {
//!	uint8_t temprature_sens_read(void);
//!}

void HandlersPooling_t::OverheatHandler::Start()  {
	IsActive 		= true;
	OverheatTimer 	= 0;
}

void HandlersPooling_t::OverheatHandler::Stop()  {
	IsActive 		= false;
	OverheatTimer 	= 0;
}

void HandlersPooling_t::OverheatHandler::Pool() {
	bool IsOverheated = false;

	if (!IsActive) return;

	OverheatTimer += Settings.Pooling.Interval;

	if (OverheatTimer < Settings.Pooling.OverHeat.Inverval)
		return;

	//!uint8_t SoCTemperature = temprature_sens_read();
	uint8_t SoCTemperature = 0;
	SoCTemperature = (uint8_t)floor((SoCTemperature - 32) * (5.0/9.0) + 0.5); // From Fahrenheit to Celsius
	Device.Temperature = SoCTemperature;

	if (SoCTemperature > Settings.Pooling.OverHeat.OverheatTemp) {
		IsOverheated = true;
		Log::Add(Log::Events::System::DeviceOverheat);

		for (auto& Command : Commands)
			Command->Overheated();
	}

	if (SoCTemperature < Settings.Pooling.OverHeat.ChilledTemp) {
		if (IsOverheated) {
			IsOverheated = false;
			Log::Add(Log::Events::System::DeviceCooled);
		}
	}

	OverheatTimer = 0;
}