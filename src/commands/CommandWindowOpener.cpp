/*
*    CommandSwitch.cpp
*    CommandSwitch_t implementation
*
*/
#include "Sensors.h"

const char CommandWOBaseTag[]			= "WindowOpenerBase";


class CommandWindowOpener_t : public Command_t {
  public:
	inline static uint8_t 	CurrentOpenProcent 				= 0;		// Текущий процент открытия 0-100%
	inline static bool 		IsLimitsModifficationEnabled	= false;	// Флаг режима изменения крайних положений

	enum OperationalModeEnum { Basic = 0x0, Callibration = 0x1, LimitsEdit = 0x2 };

	CommandWindowOpener_t() {
		ID          	= 0x10;
		Name        	= "WindowOpener";

		Events["open"]  = 0x01;
		Events["close"] = 0x02;
		Events["set"] 	= 0x03;

		Events["autocalibration"] 	= 0x10;
		Events["limits-edit"] 		= 0x11;
    }
    
    void Overheated() override {
    	// Что делаем при перегреве устройства, например для розетки - отключаем нагрузку
    	// для Drivent логично отключить двигатель, имхо
    }

	// Команда открытия окна на нужную позицию
	virtual void SetPosition(uint8_t Target, uint8_t Speed) { };
	virtual void SetLimitModeEnabledPostActions()			{ };
	virtual bool ResetMotorPosition()						{ return false; };
	virtual bool SaveOpenPosition()							{ return false; };
	virtual bool SaveClosePosition()						{ return false; };
	virtual bool AutoCallibrationStart()					{ return false; };

	void SetCurrentOpenPercent(uint8_t Value) {
		CurrentOpenProcent = Value;

		if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr)
			Sensor_t::GetSensorByID(ID + 0x80)->SetValue(Value, "Position", 0);
	}

	void SetCurrentMode(OperationalModeEnum Mode) {
		if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr)
			Sensor_t::GetSensorByID(ID + 0x80)->SetValue((uint32_t)Mode, "Mode", 0);
	}

	// Event code это один из трех uint8_t полей. открыть, закрыть или установить в процентах открытие
	bool Execute(uint8_t EventCode, const char* StringOperand) override 
	{
		// Открыть окно полностью
		if (EventCode == 0x01) {
			// Здесь может быть подача на пин двигателя 1 или запуск таска по открытию окна
			// Привет подачи 1 на пин: GPIO::Write(Settings.GPIOData.GetCurrent().Switch.GPIO, true);
			// В примере вызывается Set со 100%
			return Execute(0x03, "00000100");
		}

		// Закрыть окно полностью
		if (EventCode == 0x02) {
			// Здесь может быть подача на пин двигателя 0 или запуск таска по открытию окна
			// Привет подачи 1 на пин: GPIO::Write(Settings.GPIOData.GetCurrent().Switch.GPIO, false);
			return Execute(0x03, "00000000");
		}

		// Установить открытие на нужный %
		if (EventCode == 0x03) {
			uint16_t Position = 0;

			string Operand(StringOperand);

			if (Operand.size() > 3)
				Operand = Operand.substr(0, 3);

			Position = Converter::ToUint16(Operand);

			if (Position > 100)
				Position = 100;

			bool IsPositionTheSame = false;

			if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr) {
				uint32_t CurrentPosition = Sensor_t::GetSensorByID(ID + 0x80)->GetValue();

				if (Position == CurrentPosition)
					IsPositionTheSame = true;
			}

			// изменить угол наклона в позицию Delta
			if (!IsPositionTheSame) {
				ESP_LOGI(CommandWOBaseTag, "Position to set: %d", Position);
				SetPosition((uint8_t)Position, 25);
			}

			return true;
		}

		string Operand(StringOperand);
		Operand = Converter::ToLower(Operand);

		// Запуск авто-калибровки
		if (EventCode == 0x10) {
			SetCurrentMode(Callibration);
			return AutoCallibrationStart();
		}

		// Режим обработки изменения крайних положений
		if (EventCode == 0x11) 
		{
			//todo: отправлять в сенсор WindowOpener статус режима - работает ли настройка автоположений или нет
			if (Operand == "enable" || Operand == "disable") {
				IsLimitsModifficationEnabled = (Operand == "enable") ? true : false;
				SetLimitModeEnabledPostActions();

				SetCurrentMode((IsLimitsModifficationEnabled) ? LimitsEdit : Basic);

				return true;
			}

			if (!IsLimitsModifficationEnabled)
				return false;

			// сбросить положения привода
			if (Operand == "reset") {
				return ResetMotorPosition();
			}

			// Сохранить положение "Открыто"
			if (Operand == "open_set") {
				return SaveOpenPosition();
			}

			// Сохранить положение "Открыто"
			if (Operand == "closed_set") {
				return SaveClosePosition();
			}
		}

		return false;
	}
};
