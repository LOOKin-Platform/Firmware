/*
 *    HaierAC.cpp
 *    Class for Haier AC protocol
 *    9 or 14 state length
 *
 *    Generic		: 	Haier HSU07-HEA03 Remote control, HSU-09HMC203 A/C unit
 *    HAIER11		: 	Haier YR-H71 Remote control
 *    HAIER14		: 	Haier YR-W02 Remote control
 *
 */

#include "DateTime.h"

class HaierUnit {
	protected:
		uint8_t RemoteState[14];
	public:
		virtual ~HaierUnit() 	{}

		uint8_t		DeviceType		= 0;

		uint8_t		StateLength 	= 0;
		uint16_t	Frequency 		= 0;
		uint32_t	Gap				= 150000;

		uint16_t	HeaderMark		= 0;
		uint16_t	HeaderSpace		= 0;
		uint16_t	HeaderGap		= 0;

		uint16_t	BitMark			= 0;
		uint16_t	OneSpace		= 0;
		uint16_t	ZeroSpace		= 0;

		uint8_t		Prefix			= 0;

		vector<uint8_t> Sections= vector<uint8_t>();

		virtual void 	Erase() 	{};
		virtual void 	Checksum() 	{};

		virtual void 	SetMode(ACOperand::GenericMode) { };
		virtual ACOperand::GenericMode GetMode() { return ACOperand::GenericMode::ModeAuto; };

		virtual void 	SetTemperature(uint8_t Temperature) {};
		virtual uint8_t GetTemperature() { return 0; };

		virtual void 	SetHSwing(ACOperand::SwingMode) {};
		virtual ACOperand::SwingMode GetHSwing() { return ACOperand::SwingOff; };

		virtual void 	SetVSwing(ACOperand::SwingMode) {};
		virtual ACOperand::SwingMode GetVSwing() { return ACOperand::SwingOff; };

		virtual void 	SetFanMode(ACOperand::FanModeEnum) {};
		virtual ACOperand::FanModeEnum GetFanMode() { return ACOperand::FanAuto; };

		virtual void 	SetACCommand(ACOperand::CommandEnum) {};

		vector<int32_t> GetSignal() {
			Checksum();

			vector<int32_t> Result;

			for (int i = 0; i < Sections.size(); i++) {
				Result.push_back(HeaderMark);
				Result.push_back(-HeaderSpace);
				Result.push_back(HeaderMark);
				Result.push_back(-HeaderGap);

				int Start = 0;

				if (i == 1)
					Start += Sections[0];
				else if (i == 2)
					Start += Sections[0] + Sections[1];

				for (int j = Start; j < Start + Sections[i]; j++) {
					bitset<8> Byte(RemoteState[j]);

					ESP_LOGE("RemoteState", "[%d] = %02X", j, RemoteState[j]);

					for (int i = 7; i >= 0; i--) {
						Result.push_back(BitMark);
						Result.push_back((Byte[i] == true) ? -OneSpace : -ZeroSpace);
					}
				}

				Result.push_back(BitMark);
				Result.push_back(-Gap);
			}

			Result.push_back(-Gap);

			return Result;
		}

		bool IsProtocol(vector<int32_t> &Raw) {
			if (Sections.size() == 0)
				return false;

			if (Raw.size() == 4 + 2 + StateLength * 2 * 8)
			{
				if (IRProto::TestValue(Raw.at(0), HeaderMark) && IRProto::TestValue(Raw.at(1), -HeaderSpace) &&
					IRProto::TestValue(Raw.at(2), HeaderMark) && IRProto::TestValue(Raw.at(3), -HeaderGap ))
					return true;
			}

			return false;
		}

		ACOperand GetOperand(vector <int32_t> &Raw) {
			int ByteNum = 0;

			for (int j = 0; j < Sections.size(); j++) {
				Raw.erase(Raw.begin()); Raw.erase(Raw.begin());
				Raw.erase(Raw.begin()); Raw.erase(Raw.begin());

				for (int i = 0; i < Sections[j]; i++)
				{
					bitset<8> Byte;
					for (int k = 7; k >= 0; k--)
					{
				    	if (!IRProto::TestValue(Raw.front(), BitMark))
							return ACOperand();

				    	Raw.erase(Raw.begin());

						if      (IRProto::TestValue(Raw.front(), -OneSpace)) 	Byte[k] = 1;
						else if	(IRProto::TestValue(Raw.front(), -ZeroSpace))	Byte[k] = 0;
						else
							return ACOperand();

				    	Raw.erase(Raw.begin());
					}

					RemoteState[ByteNum++] = (uint8_t)(Byte.to_ulong());
				}

				if (Raw.size()) Raw.erase(Raw.begin());
				if (Raw.size()) Raw.erase(Raw.begin());
			}

			for (int i=0; i < StateLength; i++)
				ESP_LOGE("RemoteState", "[%d] = %s", i, Converter::ToHexString(RemoteState[i],2).c_str());

			ACOperand Operand;
			Operand.DeviceType	= DeviceType;
			Operand.Temperature	= GetTemperature();
			Operand.HSwingMode	= GetHSwing();
			Operand.VSwingMode	= GetVSwing();
			Operand.FanMode 	= GetFanMode();
			Operand.Mode		= GetMode();

			return Operand;
		}
};

class HaierGeneric : public HaierUnit {
	enum Command { 	CommandOff 		= 0b00000000, CommandOn 		= 0b00000001, CommandMode 		= 0b00000010,
					CommandFan 		= 0b00000011, CommandTempUp 	= 0b00000110, CommandTempDown 	= 0b00000111,
					CommandSleep	= 0b00001000, CommandTimerSet 	= 0b00001001, CommandTimerCancel= 0b00001010,
					CommandHealth 	= 0b00001100, CommandSwing 		= 0b00001101  };

	public:
		HaierGeneric() : HaierUnit() {
			DeviceType		= 0;

			StateLength 	= 9;
			Frequency 		= 38000;

			HeaderMark		= 3000;
			HeaderSpace		= 3000;
			HeaderGap		= 4300;

			BitMark			= 650;
			OneSpace		= 1650;
			ZeroSpace		= 520;

			Prefix			= 0xA5;

			Sections		= { 9 };
			Erase();
		}

		void Erase() override {
		  for (uint8_t i = 0; i < StateLength; i++)
			  RemoteState[i] = 0x0;

		  RemoteState[0] = Prefix;
		  RemoteState[2] = 0x20;
		  RemoteState[4] = 0x0C;
		  RemoteState[5] = 0xC0;

		  SetTemperature(25);
		  SetFanMode(ACOperand::FanAuto);
		  SetMode(ACOperand::ModeAuto);
		  SetCommand(CommandOn);

		  Checksum();
	}

	void SetMode(ACOperand::GenericMode Mode) override {
		uint8_t u8mode = 0;

		switch (Mode) {
			case ACOperand::ModeCool:	u8mode = 1; break;
			case ACOperand::ModeDry:	u8mode = 2; break;
			case ACOperand::ModeHeat:	u8mode = 3; break;
			case ACOperand::ModeFan:	u8mode = 4; break;
	    	case ACOperand::ModeAuto:
			default:
			 	u8mode = 0;
		}

		SetCommand(CommandMode);

		RemoteState[6] &= ~0b11100000;
		RemoteState[6] |= (u8mode << 5);

		if (Mode == ACOperand::ModeOff) {
			SetCommand(CommandOff);
		}
	}

	ACOperand::GenericMode GetMode() override {
		if (GetCommand() == HaierGeneric::CommandOff)
			return ACOperand::ModeOff;

		uint8_t u8mode = (RemoteState[6] & 0b11100000) >> 5;

		switch (u8mode) {
			case 1: return ACOperand::ModeCool	; break;
			case 2: return ACOperand::ModeDry	; break;
			case 3: return ACOperand::ModeHeat	; break;
			case 4: return ACOperand::ModeFan	; break;
			default:
				return ACOperand::ModeAuto;
			}
	}

	void SetTemperature(const uint8_t Temperature) override {
		uint8_t Degrees = max(Temperature, (uint8_t)16);
		Degrees = min(Degrees, (uint8_t)30);

		uint8_t OldTemp = GetTemperature();

		if (OldTemp == Degrees) return;

		SetCommand((OldTemp > Degrees) ? CommandTempDown : CommandTempUp);

		RemoteState[1] &= 0b00001111;  // Clear the previous temp.
		RemoteState[1] |= ((Degrees - 16) << 4);
	}

	uint8_t GetTemperature() override {
		  return ((RemoteState[1] & 0b11110000) >> 4) + 16;
	}

	void SetHSwing(ACOperand::SwingMode Mode) override {
	}

	ACOperand::SwingMode GetHSwing() override {
		return ACOperand::SwingOff;
	}

	void SetVSwing(ACOperand::SwingMode Mode) override {
		if (Mode == GetVSwing()) return;

		uint8_t u8swing = 0x0;

		switch (u8swing) {
			case ACOperand::SwingTop	: u8swing = 0b00000001; break;
			case ACOperand::SwingBottom	: u8swing = 0b00000010; break;
			case ACOperand::SwingAuto	:
			default:
				u8swing = 0b00000000;
				break;
		}

		SetCommand(CommandSwing);

		RemoteState[2] &= 0b00111111;
		RemoteState[2] |= (u8swing << 6);
	}

	ACOperand::SwingMode GetVSwing() override {
		uint8_t u8swing = (RemoteState[2] & 0b11000000) >> 6;

		switch (u8swing) {
			case 0b00000001: return ACOperand::SwingTop		; break;
			case 0b00000010: return ACOperand::SwingBottom	; break;
			case 0b00000000:
			default:
				return ACOperand::SwingAuto;
				break;
		}
	}

	void SetFanMode(ACOperand::FanModeEnum Mode) override {
		  uint8_t u8fan = 0x0;

		  switch (Mode) {
		    case ACOperand::Fan1:
		    case ACOperand::Fan2:
		    	Mode = ACOperand::Fan1;
		    	u8fan = 3;
		      break;
		    case ACOperand::Fan3:
		    	u8fan = 1;
		      break;
		    case ACOperand::Fan4:
		    case ACOperand::Fan5:
		    	Mode = ACOperand::Fan5;
		    	u8fan = 2;
		      break;
		    default:
		    	Mode = ACOperand::FanAuto;
		    	u8fan = 0;
		  }

		  if (Mode != GetFanMode())
			  SetCommand(CommandFan);

		  RemoteState[5] &= 0b11111100;
		  RemoteState[5] |= u8fan;
	};

	ACOperand::FanModeEnum GetFanMode() override {
		  switch (RemoteState[5] & 0b00000011) {
		    case 1:
		      return ACOperand::Fan3;
		    case 2:
		      return ACOperand::Fan5;
		    case 3:
		      return ACOperand::Fan1;
		    default:
		      return ACOperand::FanAuto;
		  }
	};

	void SetACCommand(ACOperand::CommandEnum Command) override {
		switch (Command) {
			case ACOperand::CommandOff		: SetCommand(HaierGeneric::CommandOff); break;
			case ACOperand::CommandOn		: SetCommand(HaierGeneric::CommandOn); break;
			case ACOperand::CommandMode		: SetCommand(HaierGeneric::CommandMode); break;
			case ACOperand::CommandFan		: SetCommand(HaierGeneric::CommandFan); break;
			case ACOperand::CommandTempUp 	: SetCommand(HaierGeneric::CommandTempUp); break;
			case ACOperand::CommandTempDown	: SetCommand(HaierGeneric::CommandTempDown); break;
			case ACOperand::CommandHSwing	:
			case ACOperand::CommandVSwing	: SetCommand(HaierGeneric::CommandSwing); break;
			default: break;
		}
	}

	void SetCommand(Command CommandItem)  {
		RemoteState[1] &= 0b11110000;
    	RemoteState[1] |= (static_cast<uint8_t>(CommandItem) & 0b00001111);
	};

	Command	GetCommand() {
		return static_cast<Command>(RemoteState[1] & (0b00001111));
	};

	void Checksum() override {
		RemoteState[8] = Converter::SumBytes(RemoteState, StateLength - 1);
	}
};

class Haier11 : public HaierUnit {
	enum Command { 	CommandOff 		= 0b00000000, CommandOn 		= 0b00000001, CommandMode 		= 0b00000010,
					CommandFan 		= 0b00000011, CommandTempUp 	= 0b00000110, CommandTempDown 	= 0b00000111,
					CommandTimerSet = 0b00001001, CommandTimerCancel= 0b00001010,
					CommandHealth 	= 0b00001100, CommandSwing 		= 0b00001101  };

	public:
		Haier11() : HaierUnit() {
			DeviceType		= 11;

			StateLength 	= 11;
			Frequency 		= 38000;

			HeaderMark		= 3000;
			HeaderSpace		= 3000;
			HeaderGap		= 4300;

			BitMark			= 620;
			OneSpace		= 1650;
			ZeroSpace		= 520;

			Prefix			= 0xA6;

			Sections		= { 11 };
			Erase();
		}

		void Erase() override {
		  for (uint8_t i = 0; i < StateLength; i++)
			  RemoteState[i] = 0x0;

		  RemoteState[0] = Prefix;
		  RemoteState[3] = 0x32;
		  RemoteState[8] = 0xF2;

		  SetTemperature(25);
		  SetFanMode(ACOperand::FanAuto);
		  SetMode(ACOperand::ModeAuto);
		  SetCommand(CommandOn);

		  Checksum();
	}

	void SetMode(ACOperand::GenericMode Mode) override {
		uint8_t u8mode = 0;

		switch (Mode) {
			case ACOperand::ModeCool:	u8mode = 1; break;
			case ACOperand::ModeDry:	u8mode = 2; break;
			case ACOperand::ModeHeat:	u8mode = 3; break;
			case ACOperand::ModeFan:	u8mode = 4; break;
	    	case ACOperand::ModeAuto:
			default:
			 	u8mode = 0;
		}

		SetCommand(CommandMode);

		RemoteState[6] &= ~0b11100000;
		RemoteState[6] |= (u8mode << 5);

		if (Mode == ACOperand::ModeOff)
			SetCommand(CommandOff);
	}

	ACOperand::GenericMode GetMode() override {
		if (GetCommand() == Haier11::CommandOff)
			return ACOperand::ModeOff;

		uint8_t u8mode = (RemoteState[6] & 0b11100000) >> 5;

		switch (u8mode) {
			case 1: return ACOperand::ModeCool	; break;
			case 2: return ACOperand::ModeDry	; break;
			case 3: return ACOperand::ModeHeat	; break;
			case 4: return ACOperand::ModeFan	; break;
			default:
				return ACOperand::ModeAuto;
			}
	}

	void SetTemperature(const uint8_t Temperature) override {
		uint8_t Degrees = max(Temperature, (uint8_t)16);
		Degrees = min(Degrees, (uint8_t)30);

		uint8_t OldTemp = GetTemperature();

		if (OldTemp == Degrees) return;

		SetCommand((OldTemp > Degrees) ? CommandTempDown : CommandTempUp);

		RemoteState[1] &= 0b00001111;  // Clear the previous temp.
		RemoteState[1] |= ((Degrees - 16) << 4);
	}

	uint8_t GetTemperature() override {
		  return ((RemoteState[1] & 0b11110000) >> 4) + 16;
	}

	void SetHSwing(ACOperand::SwingMode Mode) override {
	}

	ACOperand::SwingMode GetHSwing() override {
		return ACOperand::SwingOff;
	}

	void SetVSwing(ACOperand::SwingMode Mode) override {
		if (Mode == GetVSwing()) return;

		uint8_t u8swing = 0x0;

		switch (u8swing) {
			case ACOperand::SwingTop	: u8swing = 0b00000001; break;
			case ACOperand::SwingBottom	: u8swing = 0b00000010; break;
			case ACOperand::SwingAuto	:
			default:
				u8swing = 0b00000000;
				break;
		}

		SetCommand(CommandSwing);

		RemoteState[2] &= 0b00111111;
		RemoteState[2] |= (u8swing << 6);
	}

	ACOperand::SwingMode GetVSwing() override {
		uint8_t u8swing = (RemoteState[2] & 0b11000000) >> 6;

		switch (u8swing) {
			case 0b00000001: return ACOperand::SwingTop		; break;
			case 0b00000010: return ACOperand::SwingBottom	; break;
			case 0b00000000:
			default:
				return ACOperand::SwingAuto;
				break;
		}
	}

	void SetFanMode(ACOperand::FanModeEnum Mode) override {
		  uint8_t u8fan = 0x0;

		  switch (Mode) {
		  	  case ACOperand::Fan1:
		  	  case ACOperand::Fan2:
		  		  Mode = ACOperand::Fan1;
		  		  u8fan = 3;
		  		  break;
		  	  case ACOperand::Fan3:
		  		  u8fan = 1;
		  		  break;
		  	  case ACOperand::Fan4:
		  	  case ACOperand::Fan5:
		  		  Mode = ACOperand::Fan5;
		  		  u8fan = 2;
		  		  break;
		  	  default:
		  		  Mode = ACOperand::FanAuto;
		  		  u8fan = 0;
		  }

		  if (Mode != GetFanMode())
			  SetCommand(CommandFan);

		  RemoteState[4] &= 0b11100000;
		  RemoteState[4] |= (u8fan << 5);
	};

	ACOperand::FanModeEnum GetFanMode() override {
		uint8_t u8fan = (RemoteState[4] >> 5);

		switch (u8fan) {
			case 3: return ACOperand::Fan1; break;
		    case 2: return ACOperand::Fan3; break;
		    case 1: return ACOperand::Fan5; break;
		    default:
		    	return ACOperand::FanAuto;
		  }
	};

	void SetACCommand(ACOperand::CommandEnum Command) override {
		switch (Command) {
			case ACOperand::CommandOff		: SetCommand(Haier11::CommandOff); break;
			case ACOperand::CommandOn		: SetCommand(Haier11::CommandOn); break;
			case ACOperand::CommandMode		: SetCommand(Haier11::CommandMode); break;
			case ACOperand::CommandFan		: SetCommand(Haier11::CommandFan); break;
			case ACOperand::CommandTempUp 	: SetCommand(Haier11::CommandTempUp); break;
			case ACOperand::CommandTempDown	: SetCommand(Haier11::CommandTempDown); break;
			case ACOperand::CommandHSwing	:
			case ACOperand::CommandVSwing	: SetCommand(Haier11::CommandSwing); break;
			default: break;
		}
	}

	void SetCommand(Command CommandItem)  {
		RemoteState[1] &= 0b11110000;
    	RemoteState[1] |= (static_cast<uint8_t>(CommandItem) & 0b00001111);
	};

	Command	GetCommand() {
		return static_cast<Command>(RemoteState[1] & (0b00001111));
	};

	void Checksum() override {
		RemoteState[10] = Converter::SumBytes(RemoteState, StateLength - 1);
	}
};

class Haier14 : public HaierUnit {
	enum Command { 	CommandTempUp 	= 0x0, CommandTempDown 		= 0x1, CommandSwing = 0x2,
					CommandFan 		= 0x4, CommandPower			= 0x5, CommandMode	= 0x6,
					CommandHealth	= 0x7, CommandTurbo 		= 0x8, CommandSleep	= 0xB };

	enum Turbo 	 { TurboOff = 0x0, TurboHigh = 0x1, TurboLow = 0x2 };

	public:
		Haier14() : HaierUnit() {
			DeviceType		= 14;

			StateLength 	= 14;
			Frequency 		= 38000;

			HeaderMark		= 3000;
			HeaderSpace		= 3000;
			HeaderGap		= 4300;

			BitMark			= 650;
			OneSpace		= 1650;
			ZeroSpace		= 520;

			Prefix			= 0xA6;

			Sections		= { 14 };
			Erase();
		}

		void Erase() override {
			for (uint8_t i = 1; i < StateLength; i++)
				RemoteState[i] = 0x0;

			RemoteState[0] = Prefix;

			SetTemperature(25);
			SetHealth(true);
			SetTurbo(Haier14::TurboOff);
			SetSleep(false);
			SetFanMode(ACOperand::FanAuto);
			SetVSwing(ACOperand::SwingOff);
			SetMode(ACOperand::ModeAuto);
			SetPower(true);

			Checksum();
		}

		bool GetPower(void) {
			return RemoteState[4] & 0b01000000;
		}

		void SetPower(bool State) {
			SetCommand(Haier14::CommandPower);

			if (State)
				RemoteState[4] |= 0b01000000;
			else
				RemoteState[4] &= ~0b01000000;
		}

		void SetMode(ACOperand::GenericMode Mode) override {
			uint8_t u8mode = 0x0;

			if  (Mode == ACOperand::ModeOff) {
				SetPower(false);
				return;
			}

			switch (Mode) {
				case ACOperand::ModeAuto: u8mode = 0x0; break;
				case ACOperand::ModeCool: u8mode = 0x2; break;
				case ACOperand::ModeDry	: u8mode = 0x4; break;
				case ACOperand::ModeHeat: u8mode = 0x8; break;
				case ACOperand::ModeFan	: u8mode = 0xC; break;
				default:
					u8mode = 0x0;
			}

			SetCommand(Haier14::CommandMode);

			RemoteState[7] &= 0b0001111;
			RemoteState[7] |= (u8mode << 4);
		}

		ACOperand::GenericMode GetMode() override {
			if (!GetPower())
				return ACOperand::ModeOff;

			uint8_t u8mode = RemoteState[7] >> 4;

			switch (u8mode) {
				case 0x2: return ACOperand::ModeCool;
				case 0x4: return ACOperand::ModeDry;
				case 0x8: return ACOperand::ModeHeat;
				case 0xC: return ACOperand::ModeFan;
				default:
					return ACOperand::ModeAuto;
			}
		}

		void SetTemperature(const uint8_t Temperature) override {
			uint8_t Degrees = max((uint8_t)16, Temperature);
			Degrees = min((uint8_t)30, Temperature);

			uint8_t OldTemp = GetTemperature();

			if (OldTemp == Degrees) return;

			SetCommand((OldTemp > Degrees) ? Haier14::CommandTempDown : Haier14::CommandTempUp);

			RemoteState[1] &= 0b00001111;
			RemoteState[1] |= ((Degrees - 16) << 4);
		}

		uint8_t GetTemperature() override {
			return ((RemoteState[1] & 0b11110000) >> 4) + 16;
		}

		void SetHSwing(ACOperand::SwingMode Mode) override {
		}

		ACOperand::SwingMode GetHSwing() override {
			return ACOperand::SwingOff;
		}

		void SetVSwing(ACOperand::SwingMode Mode) override {
			uint8_t u8swing = 0x0;

			switch (Mode) {
				case ACOperand::SwingOff	: u8swing = 0x0; break;
				case ACOperand::SwingTop	: u8swing = 0x1; break;
				case ACOperand::SwingMiddle	: u8swing = 0x2; break;
				case ACOperand::SwingBottom	: u8swing = 0x3; break;
				case ACOperand::SwingDown	: u8swing = 0xA; break;
				default:
					u8swing = 0xC; // auto
			}

			SetCommand(Haier14::CommandSwing);

			if (Mode == ACOperand::SwingMiddle && GetMode() == ACOperand::ModeHeat)
				u8swing = 0x3;

			// BOTTOM is only allowed if we are in Heat mode, otherwise MIDDLE.
			if (Mode == ACOperand::SwingBottom && GetMode() != ACOperand::ModeHeat)
			    u8swing = 0x2;

			RemoteState[1] &= 0b11110000;
			RemoteState[1] |= u8swing;
		}

		ACOperand::SwingMode GetVSwing() override {
			uint8_t Position = RemoteState[1] & 0b00001111;

			switch (Position) {
				case 0x0: return ACOperand::SwingOff; 		break;
				case 0x1: return ACOperand::SwingTop; 		break;
				case 0x2: return ACOperand::SwingMiddle; 	break;
				case 0x3: return ACOperand::SwingBottom; 	break;
				case 0xA: return ACOperand::SwingDown; 		break;
				default:
					return ACOperand::SwingAuto;
			}
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) override {
			uint8_t u8fan = 0x0;

			switch (Mode) {
				case ACOperand::Fan1:
				case ACOperand::Fan2:
				case ACOperand::FanQuite:
					u8fan = 0x6;
					break;
				case ACOperand::Fan3:
					u8fan = 0x4;
					break;
				case ACOperand::Fan4:
				case ACOperand::Fan5:
					u8fan = 0x2;
					break;
				default:
					u8fan = 0xA; //auto
			}

			RemoteState[5] &= 0b00001111;
			RemoteState[5] |= (u8fan << 4);
			SetCommand(Haier14::CommandFan);

		};

		ACOperand::FanModeEnum GetFanMode() override {
			uint8_t u8fan = RemoteState[5] >> 4;

			switch (u8fan) {
				case 0x6: return ACOperand::Fan1;
				case 0x4: return ACOperand::Fan3;
				case 0x2: return ACOperand::Fan5;
				default:
					return ACOperand::FanAuto;
			}
		};

		void SetACCommand(ACOperand::CommandEnum Command) override {
			switch (Command) {
				case ACOperand::CommandMode		: SetCommand(Haier14::CommandMode); break;
				case ACOperand::CommandFan		: SetCommand(Haier14::CommandFan); break;
				case ACOperand::CommandTempUp 	: SetCommand(Haier14::CommandTempUp); break;
				case ACOperand::CommandTempDown	: SetCommand(Haier14::CommandTempDown); break;
				case ACOperand::CommandHSwing	:
				case ACOperand::CommandVSwing	: SetCommand(Haier14::CommandSwing); break;
				default: break;
			}
		}

		void SetCommand(Command CommandItem)  {
			RemoteState[12] &= 0b11110000;
			RemoteState[12] |= (static_cast<uint8_t>(CommandItem) & 0b00001111);
		};

		Command	GetCommand() {
			return static_cast<Command>(RemoteState[1] & (0b00001111));
		};

		void SetSleep(bool State) {
			SetCommand(Haier14::CommandSleep);

			if (State)
				RemoteState[8] |= 0b10000000;
			else
			  RemoteState[8] &= ~0b10000000;
		}

		bool GetSleep(void) {
		  return RemoteState[8] & 0b10000000;
		}

		void SetHealth(bool State) {
			SetCommand(Haier14::CommandHealth);
			RemoteState[4] &= 0b11011111;
			RemoteState[4] |= (State << 5);
		}

		bool GetHealth(void) {
			return RemoteState[4] & (1 << 5);
		}

		void SetTurbo(Turbo State) {
			uint8_t u8turbo = static_cast<uint8_t>(State);

			RemoteState[6] &= 0b00111111;
			RemoteState[6] |= (u8turbo << 6);
			SetCommand(Haier14::CommandTurbo);
		}

		Turbo GetTurbo(void) {
			uint8_t u8turbo = RemoteState[6] >> 6;

			return static_cast<Turbo>(u8turbo);
		}

		void Checksum() override {
			RemoteState[StateLength - 1] = Converter::SumBytes(RemoteState, StateLength - 1);
		}
};

class HaierAC : public IRProto {
	private:
		vector<HaierUnit*> Units;
	public:
		HaierAC () {
			ID 					= 0x0B;
			Name 				= "Haier-AC";
			DefinedFrequency	= 38000;

			if (Units.size() == 0)
				Units = {
					new HaierGeneric(),
					new Haier11(),
					new Haier14()
				};
		};

		~HaierAC() {
			for (auto& Unit : Units)
				if (Unit != nullptr)
					delete (Unit);
		}

		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() < 150)
				return false;

			for (auto &Unit: Units)
				if (Unit->IsProtocol(RawData))
					return true;

			return false;
		}

		HaierUnit* GetUnit(uint8_t Code) {
			for (auto &Unit : Units)
				if (Unit->DeviceType == Code)
					return Unit;

			return nullptr;
		}

		HaierUnit* GetUnit(vector<int32_t> &Raw) {
			for (auto &Unit : Units)
				if (Unit->IsProtocol(Raw))
					return Unit;

			return nullptr;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) override {
			ACOperand AC(Data, Misc);
			HaierUnit* Unit = GetUnit(AC.DeviceType);

			if (Unit == nullptr)
				return vector<int32_t>();

			Unit->SetTemperature(AC.Temperature);
			Unit->SetHSwing(AC.HSwingMode);
			Unit->SetVSwing(AC.VSwingMode);

			Unit->SetMode(AC.Mode);

			if (AC.Command != ACOperand::CommandNULL)
				Unit->SetACCommand(AC.Command);

			DefinedFrequency = Unit->Frequency;

			return Unit->GetSignal();
		}

		pair<uint32_t,uint16_t>	GetData(vector<int32_t> Raw) override {
			HaierUnit* Unit = GetUnit(Raw);

			if (Unit == nullptr)
				return make_pair(0x0,0x0);

			return make_pair(Unit->GetOperand(Raw).ToUint32(), 0x0);
		}
};
