/*
*    CommandSwitch.cpp
*    CommandSwitch_t implementation
*
*/
#include "CommandsWindowOpener.h"

CommandWindowOpener_t::CommandWindowOpener_t() {
	ID          	= 0x10;
	Name        	= "WindowOpener";

	Events["open"]  = 0x01;
	Events["close"] = 0x02;
	Events["set"] 	= 0x03;

	Events["autocalibration"] 	= 0x10;
	Events["limits-edit"] 		= 0x11;
}

void CommandWindowOpener_t::Overheated() {
	// Что делаем при перегреве устройства, например для розетки - отключаем нагрузку
	// для Drivent логично отключить двигатель, имхо
}

void CommandWindowOpener_t::SetCurrentOpenPercent(uint8_t Value) {
	UpdateSensorWindowOpener(Value);
}

void CommandWindowOpener_t::SetCurrentOpenFinished(uint8_t Value) {
	UpdateSensorWindowOpener(Value, true);
}

void CommandWindowOpener_t::UpdateSensorWindowOpener(uint8_t Value, bool ForceUpdate) {
	CurrentOpenProcent = Value;

	if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr) 
	{
		Sensor_t::GetSensorByID(ID + 0x80)->SetValue(Value, "Position", 0);

		if (abs((int)LastPosition - (int)Value) > 9 || ForceUpdate) {
			Sensor_t::GetSensorByID(ID + 0x80)->Update();
			LastPosition = Value;
		}
	}
}

void SetCurrentMode(OperationalModeEnum Mode) {
	if (Sensor_t::GetSensorByID(ID + 0x80) != nullptr) {
		Sensor_t::GetSensorByID(ID + 0x80)->SetValue((uint32_t)Mode, "Mode", 0);
		Sensor_t::GetSensorByID(ID + 0x80)->Update();
	}
}

// Event code это один из трех uint8_t полей. открыть, закрыть или установить в процентах открытие
bool CommandWindowOpener_t::Execute(uint8_t EventCode, const char* StringOperand) {
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
			SetPosition((uint8_t)Position);
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
		if (Operand == "close_set") {
			return SaveClosePosition();
		}
	}

	return false;
}