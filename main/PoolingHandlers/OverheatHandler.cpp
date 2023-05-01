#include "HandlersPooling.h"

#include "Log.h"
#include "FreeRTOSWrapper.h"

#if CONFIG_IDF_TARGET_ESP32
extern "C" {
	uint8_t temprature_sens_read(void);
}
#endif

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

#if CONFIG_IDF_TARGET_ESP32
	uint8_t SoCTemperature = temprature_sens_read();
#elif CONFIG_IDF_TARGET_ESP32C6
	uin8_t SoCTemperature = 0;
#endif

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