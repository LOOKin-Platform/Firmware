#include "FreeRTOSWrapper.h"
#include "Log.h"

extern "C" {
	uint8_t temprature_sens_read(void);
}

class OverheatHandler
{
	public:
		static void Start();
		static void Stop();
	private:
		static void CheckOverheat(void *pvParameters);
		static TaskHandle_t CheckOverheatTaskHandle;
};

TaskHandle_t OverheatHandler::CheckOverheatTaskHandle =  NULL;

void OverheatHandler::Start()  {
	CheckOverheatTaskHandle = FreeRTOS::StartTaskPinnedToCore(OverheatHandler::CheckOverheat, "CheckOverheat", NULL, 2048);
}

void OverheatHandler::Stop()  {
	FreeRTOS::DeleteTask(CheckOverheatTaskHandle);
}

void OverheatHandler::CheckOverheat(void *pvParameters) {
	bool IsOverheated = false;

	while (1) {
		uint8_t ChipTemperature = temprature_sens_read();
		ChipTemperature = (uint8_t)floor((ChipTemperature - 32) * (5.0/9.0) + 0.5); // From Fahrenheit to Celsius
		Device->Temperature = ChipTemperature;

		if (ChipTemperature > 90) {
			IsOverheated = true;
			Log::Add(LOG_DEVICE_OVERHEAT);
			for (auto& Command : Commands)
				Command->Overheated();
    		}

		if (ChipTemperature < 77) {
			if (IsOverheated) {
				IsOverheated = false;
				Log::Add(LOG_DEVICE_COOLLED);
			}
		}

		FreeRTOS::Sleep(OVERHEET_POOLING);
	}
}
