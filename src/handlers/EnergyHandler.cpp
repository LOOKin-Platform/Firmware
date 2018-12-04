#ifndef ENERGY_HANDLER
#define ENERGY_HANDLER

#include "esp_adc_cal.h"

static esp_adc_cal_characteristics_t *adc_chars;

class EnergyPeriodicHandler {
	public:
		static void Init();
		static void Pool();
	private:
		static bool IsInited;
};

bool EnergyPeriodicHandler::IsInited = false;

void EnergyPeriodicHandler::Init() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_11db);

    adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));

    IsInited = true;
}

void EnergyPeriodicHandler::Pool() {
	if (!IsInited)
		Init();

	if (Time::Uptime() %5 == 0) {
	    uint16_t USBValue	= (uint16_t)adc1_get_raw(ADC1_CHANNEL_5);
	    uint16_t BATValue	= (uint16_t)adc1_get_raw(ADC1_CHANNEL_4);

	    uint32_t voltage = ::esp_adc_cal_raw_to_voltage(BATValue, adc_chars);

		if (USBValue > 3072)
			Device.PowerMode = DevicePowerMode::CONST;
		else
			Device.PowerMode = DevicePowerMode::BATTERY;
	}

}

#endif
