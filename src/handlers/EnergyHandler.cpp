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

	if (Device.Type.Hex != Settings.Devices.Remote)
		return;

	if (Time::Uptime() %5 == 0) {
	    uint16_t ConstValueSrc		= 0;
	    uint16_t BatteryValueSrc	= 0;

		if (Settings.GPIOData.GetCurrent().PowerMeter.ConstPowerChannel != ADC1_CHANNEL_MAX)
			ConstValueSrc	= (uint16_t)adc1_get_raw(Settings.GPIOData.GetCurrent().PowerMeter.ConstPowerChannel);

		if (Settings.GPIOData.GetCurrent().PowerMeter.BatteryPowerChannel != ADC1_CHANNEL_MAX)
			BatteryValueSrc	= (uint16_t)adc1_get_raw(Settings.GPIOData.GetCurrent().PowerMeter.BatteryPowerChannel);

		uint16_t ConstValue 		= Converter::VoltageFromADC(ConstValueSrc, 51, 100 );
		uint16_t BatteryValue 		= Converter::VoltageFromADC(BatteryValueSrc, 100, 51 );

		if (ConstValue > 5000) {
			if (Settings.eFuse.Type == Settings.Devices.Remote && Device.PowerMode == DevicePowerMode::BATTERY)
				PowerManagement::SetIsActive(false);

			Device.PowerMode = DevicePowerMode::CONST;
		}
		else {
			if (Settings.eFuse.Type == Settings.Devices.Remote && Device.PowerMode == DevicePowerMode::CONST)
				PowerManagement::SetIsActive(true);

			Device.PowerMode = DevicePowerMode::BATTERY;
		}

		Device.CurrentVoltage = (Device.PowerMode == DevicePowerMode::CONST) ? ConstValue : BatteryValue;
	}

}

#endif
