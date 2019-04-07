/*
*    SensorTemperature.cpp
*    SensorTemperature_t implementation
*
*/

#include "I2C.h"

extern "C" {
	uint8_t temprature_sens_read(void);
}

class SensorTemperature_t : public Sensor_t {
	private:
		uint32_t PreviousValue = 0;

	public:
		I2C bus;

		SensorTemperature_t() {
			if (GetIsInited()) return;

			ID          = 0x85;
			Name        = "Temperature";
			EventCodes  = { 0x00, 0x01, 0x02 };

			if (Settings.GPIOData.GetCurrent().Temperature.I2CAddress > 0x00)
				bus.Init(Settings.GPIOData.GetCurrent().Temperature.I2CAddress);

			PreviousValue = ReceiveValue();

			SetIsInited(true);
		}

		void Update() override {
			if (SetValue(ReceiveValue())) {
				Wireless.SendBroadcastUpdated(ID, Converter::ToHexString(GetValue().Value,4));
				Automation.SensorChanged(ID);
			}
		};

		void Pool() override {
			uint32_t NewValue = ReceiveValue();

			if (NewValue != GetValue().Value) {
				PreviousValue = GetValue().Value;
				Update();
			}
		}

		uint32_t ReceiveValue(string Key = "Primary") override {
			float 		Temperature = GetTemperature();
			uint32_t  	Value		= (Temperature < 0) ? 0x1000 : 0x0000;

			Value	+= (uint32_t)abs(Temperature * 10);
			return Value;
		};

		string FormatValue(string Key = "Primary") override {
			if (Key == "Primary")
				return (Values[Key].Value >= 0x1000)
						? "-" + Converter::ToString(((float)(Values[Key].Value-0x1000) / 10))
						: Converter::ToString(((float)Values[Key].Value / 10));

			return Converter::ToHexString(Values[Key].Value, 2);
		}

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {

			float Previous 	= ConvertToFloat(PreviousValue);
			float Current 	= ConvertToFloat(GetValue().Value);
			float Operand	= ConvertToFloat(SceneEventOperand);

			if (SceneEventCode == 0x02 && Previous <= Operand && Current > Operand)
				return true;

			if (SceneEventCode == 0x03 && Previous >= Operand && Current < Operand)
				return true;

			return false;
		}

		float ConvertToFloat(uint32_t Temperature)
		{
			bool IsNegative = false;

			if (Temperature >= 0x1000)
				IsNegative = true;

			return (IsNegative) ? -((float)(Temperature - 0x1000) / 10) : (float)Temperature / 10;
		}

		float ConvertToFloat(uint8_t Temperature)
		{
			return (Temperature >= 127) ? (Temperature - 127) : (127 - Temperature);
		}

		float GetTemperature() {
			if (Settings.GPIOData.GetCurrent().Temperature.I2CAddress > 0x00)
			{
				bus.BeginTransaction();
				bus.Write(0x0);
				bus.EndTransaction();

				bus.BeginTransaction();
				uint8_t* byte = (uint8_t*) malloc(2);
				bus.Read(byte, 2, false);
				bus.EndTransaction();

				float temp = byte[0];
				if (byte[0] >= 0x80)
					temp = - (temp - 0x80);

				if (byte[1] == 0x80)
					temp += 0.5;

				if (Settings.eFuse.Type == Settings_t::Devices_t::Remote && Settings.eFuse.Revision == 1)
					temp -= 7.5;

				return temp;
			}
			else
			{
				uint8_t ChipTemperature = temprature_sens_read();
				return floor(((ChipTemperature - 32) * (5.0/9.0) + 0.5)*10)/10; // From Fahrenheit to Celsius
			}
		}
};
