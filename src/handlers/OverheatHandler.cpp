#include "Log.h"
#include "FreeRTOSWrapper.h"

extern "C" {
	uint8_t temprature_sens_read(void);
}

static bool 	IsActive 		= true;
static uint16_t	OverheatTimer	= 0;

class OverheatHandler
{
	public:
		static void Start();
		static void Stop();
		static void Pool();
};

void OverheatHandler::Start()  {
	IsActive 		= true;
	OverheatTimer 	= 0;
}

void OverheatHandler::Stop()  {
	IsActive 		= false;
	OverheatTimer 	= 0;
}

void OverheatHandler::Pool() {
	bool IsOverheated = false;

	if (!IsActive) return;

	OverheatTimer += Settings.Pooling.Interval;

	if (OverheatTimer < Settings.Pooling.OverHeat.Inverval)
		return;

	uint8_t ChipTemperature = temprature_sens_read();
	ChipTemperature = (uint8_t)floor((ChipTemperature - 32) * (5.0/9.0) + 0.5); // From Fahrenheit to Celsius
	Device.Temperature = ChipTemperature;

	if (ChipTemperature > Settings.Pooling.OverHeat.OverheatTemp) {
		IsOverheated = true;
		Log::Add(LOG_DEVICE_OVERHEAT);

		for (auto& Command : Commands)
			Command->Overheated();
	}

	if (ChipTemperature < Settings.Pooling.OverHeat.ChilledTemp) {
		if (IsOverheated) {
			IsOverheated = false;
			Log::Add(LOG_DEVICE_COOLLED);
		}
	}

	OverheatTimer = 0;
}
