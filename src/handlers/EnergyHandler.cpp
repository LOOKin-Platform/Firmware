#ifndef ENERGY_HANDLER
#define ENERGY_HANDLER

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

    IsInited = true;
}

void EnergyPeriodicHandler::Pool() {
	if (!IsInited)
		Init();

	if (Time::Uptime() %5 == 0) {
	    uint16_t USBValue	= (uint16_t)adc1_get_raw(ADC1_CHANNEL_5);
	    uint16_t BATValue	= (uint16_t)adc1_get_raw(ADC1_CHANNEL_4);

		if (USBValue > 620)
			Device.PowerMode = DevicePowerMode::CONST;
		else
			Device.PowerMode = DevicePowerMode::BATTERY;
	}

}

#endif
