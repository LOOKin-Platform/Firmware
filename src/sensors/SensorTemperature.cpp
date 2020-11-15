/*
*    SensorTemperature.cpp
*    SensorTemperature_t implementation
*
*/

class SensorTemperature_t : public Sensor_t {
	private:
		uint32_t 	PreviousValue 	= 0;

	public:
		SensorTemperature_t() {
			if (GetIsInited()) return;

			ID          = 0x85;
			Name        = "Temp";
			EventCodes  = { 0x00, 0x01, 0x02 };

			SetValue(0);

			SetIsInited(true);
		}

		void Update() override {
			Wireless.SendBroadcastUpdated(ID, Converter::ToHexString(GetValue().Value,4));
			Automation.SensorChanged(ID);
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

		string SummaryJSON() override {
			return "\"" + FormatValue() + "\"";
		};


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
};
