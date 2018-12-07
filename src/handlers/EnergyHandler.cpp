#ifndef ENERGY_HANDLER
#define ENERGY_HANDLER

#define ADCMAX				4095 //Max value at ADC_WIDTH_BIT_12
#define ADCMIN				0
#define VOLTAGEMAX			3900 //Reference voltage at ADC_ATTEN_11db
#define VOLTAGEMIN  		0
#define ADC_TO_VOLTAGE( ADCx )  ( VOLTAGEMIN + (VOLTAGEMAX-VOLTAGEMIN)*( (ADCx)-ADCMIN )/(ADCMAX-ADCMIN) )
#define VOLTAGE_BEFORE_DIVIDER( VOLTAGE, R1, R2 )   ( VOLTAGE ) * ( (R1) + (R2) ) / (R2)

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
	    uint16_t USBValueSrc	= (uint16_t)adc1_get_raw(ADC1_CHANNEL_5);
	    uint16_t BATValueSrc	= (uint16_t)adc1_get_raw(ADC1_CHANNEL_4);

		uint16_t USBValue 		= VOLTAGE_BEFORE_DIVIDER( ADC_TO_VOLTAGE(USBValueSrc), 51, 100 );
		uint16_t BATValue 		= VOLTAGE_BEFORE_DIVIDER( ADC_TO_VOLTAGE(BATValueSrc), 100, 51 );

		if (USBValue > 5000)
			Device.PowerMode = DevicePowerMode::CONST;
		else
			Device.PowerMode = DevicePowerMode::BATTERY;
	}

}

#endif
