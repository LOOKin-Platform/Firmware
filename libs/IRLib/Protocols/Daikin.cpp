/*
 *    Daikin.cpp
 *    Class for Daikin protocol
 *
 *    GENERIC		: 	ARC433** remote, FTXZ25NV1B A/C, FTXZ35NV1B A/C, FTXZ50NV1B A/C, FTE12HV2S A/C
 *    DAIKIN2		: 	ARC477A1
 *    DAIKIN128		: 	BRC52B63, 17 Series A/C, FTXB12AXVJU A/C, FTXB09AXVJU A/C
 *    DAIKIN152		: 	ARC480A5
 *    DAIKIN160		: 	ARC423A5
 *    DAIKIN176		: 	BRC4C153
 *    DAIKIN216		: 	ARC433B69
 *
 */

class DaikinUnit {
	protected:
		uint8_t RemoteState[40];
	public:
		virtual ~DaikinUnit() 	{}

		uint8_t		DeviceType		= 0;

		uint8_t		StateLength 	= 0;
		uint16_t	Frequency 		= 0;
		uint16_t	Gap				= 0;

		uint16_t	HeaderMark		= 0;
		uint16_t	HeaderSpace		= 0;
		uint16_t	BitMark			= 0;
		uint16_t	OneSpace		= 0;
		uint16_t	ZeroSpace		= 0;

		vector<uint8_t> Sections= vector<uint8_t>();

		virtual void 	Erase() 	{};
		virtual void 	Checksum() 	{};

		ACOperand::GenericMode ModeConverter(uint8_t BinaryMode) {
			switch (BinaryMode) {
				case 0b000: return ACOperand::ModeAuto; break;
				case 0b010: return ACOperand::ModeDry; 	break;
				case 0b011: return ACOperand::ModeCool; break;
				case 0b100: return ACOperand::ModeHeat; break;
				case 0b110: return ACOperand::ModeFan; 	break;
				default:
					return ACOperand::ModeAuto;
			}
		}

		uint8_t ModeConverter(ACOperand::GenericMode Mode) {
			switch (Mode) {
				case ACOperand::ModeAuto: 	return 0b000; break;
				case ACOperand::ModeDry	: 	return 0b010; break;
				case ACOperand::ModeCool: 	return 0b011; break;
				case ACOperand::ModeHeat: 	return 0b100; break;
				case ACOperand::ModeFan: 	return 0b110; break;
				default:
					return 0b000;
			}
		}

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

		vector<int32_t> GetSignal() {
			Checksum();

			vector<int32_t> Result;

			for (int i = 0; i < Sections.size(); i++) {
				Result.push_back(HeaderMark);
				Result.push_back(-HeaderSpace);

				int Start = 0;

				if (i == 1)
					Start += Sections[0];
				else if (i == 2)
					Start += Sections[0] + Sections[1];

				for (int j = Start; j < Start + Sections[i]; j++) {
					bitset<8> Byte(RemoteState[j]);
					for (int i = 0; i < 8; i++) {
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

			int16_t Diff = StateLength*16 + Sections.size()*2 + Sections.size() - Raw.size();
			int16_t SectionDelimeterSize = Sections[0] * 16 + 2 + 1;

			if (abs(Diff) < 3)
				if (Raw.size() > SectionDelimeterSize)
					if (Raw.at(SectionDelimeterSize) < -15000)
						return true;

			return false;
		}

		ACOperand GetOperand(vector <int32_t> &Raw) {
			int ByteNum = 0;

			for (int j = 0; j < Sections.size(); j++) {
				Raw.erase(Raw.begin()); Raw.erase(Raw.begin());

				for (int i = 0; i < Sections[j]; i++)
				{
					bitset<8> Byte;
					for (int k = 0; k < 8; k++)
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

			/*
			for (int i=0; i < StateLength; i++)
				ESP_LOGE("RemoteState", "[%d] = %s", i, Converter::ToHexString(RemoteState[i],2).c_str());
			*/

			ACOperand Operand;
			Operand.DeviceType 		= DeviceType;
			Operand.Temperature		= GetTemperature();
			Operand.HSwingMode		= GetHSwing();
			Operand.VSwingMode		= GetVSwing();
			Operand.FanMode 		= GetFanMode();
			Operand.Mode			= GetMode();

			return Operand;
		}
};

class DaikinGeneric : public DaikinUnit {
	public:
		DaikinGeneric() : DaikinUnit() {
			DeviceType		= 0;

			StateLength 	= 35;
			Frequency 		= 38000;
			Gap				= 29000;

			HeaderMark		= 3650;
			HeaderSpace		= 1623;
			BitMark			= 428;
			OneSpace		= 1280;
			ZeroSpace		= 428;

			Sections		= {8, 8, 19};

			Erase();
		}

		void Erase() override {
		  for (uint8_t i = 0; i < StateLength; i++)
			  RemoteState[i] = 0x0;

		  RemoteState[0] 	= 0x11;
		  RemoteState[1] 	= 0xDA;
		  RemoteState[2] 	= 0x27;
		  RemoteState[4] 	= 0xC5;
		  // remote[7] is a checksum byte, it will be set by checksum().
		  RemoteState[8] 	= 0x11;
		  RemoteState[9] 	= 0xDA;
		  RemoteState[10] 	= 0x27;
		  RemoteState[12] 	= 0x42;
		  // remote[15] is a checksum byte, it will be set by checksum().
		  RemoteState[16] 	= 0x11;
		  RemoteState[17] 	= 0xDA;
		  RemoteState[18] 	= 0x27;
		  RemoteState[21] 	= 0x49;
		  RemoteState[22] 	= 0x1E;
		  RemoteState[24]	= 0xB0;
		  RemoteState[27]	= 0x06;
		  RemoteState[28]	= 0x60;
		  RemoteState[31]	= 0xC0;
		  // remote[34] is a checksum byte, it will be set by checksum().
		  Checksum();
	}

	void SetMode(ACOperand::GenericMode Mode) override {
		uint8_t u8mode = ModeConverter(Mode);

		switch (Mode) {
	    	case ACOperand::ModeAuto:
			case ACOperand::ModeCool:
			case ACOperand::ModeHeat:
			case ACOperand::ModeFan:
			case ACOperand::ModeDry:
				RemoteState[21] &= 0b10001111;
				RemoteState[21] |= (u8mode << 4);
				break;
			default:
				SetMode(ACOperand::ModeAuto);
		}

		if (Mode == ACOperand::ModeOff) {
			RemoteState[21] &= ~0b00000001;
			return;
		}
	}

	ACOperand::GenericMode GetMode() override {
		if (!(RemoteState[21] & 0b00000001))
			return ACOperand::ModeOff;

		return ModeConverter(RemoteState[21] >> 4);
	}

	void SetTemperature(const uint8_t Temperature) override {
		uint8_t Degrees = (std::max)(Temperature, (uint8_t)10);
		Degrees = (std::min)(Degrees, (uint8_t)32);
		RemoteState[22] = Degrees << 1;
	}

	uint8_t GetTemperature() override {
		return RemoteState[22] >> 1;
	}

	void SetHSwing(ACOperand::SwingMode Mode) override {
		if (Mode == ACOperand::SwingOff)
			RemoteState[25] &= 0xF0;
		else
			RemoteState[25] |= 0x0F;

	}

	ACOperand::SwingMode GetHSwing() override {
		if (RemoteState[25] & 0x0F)
			return ACOperand::SwingAuto;
		else
			return ACOperand::SwingOff;
	}

	void SetVSwing(ACOperand::SwingMode Mode) override {
		  if (Mode == ACOperand::SwingOff)
			  RemoteState[24] &= 0xF0;
		  else
			  RemoteState[24] |= 0x0F;
	}

	ACOperand::SwingMode GetVSwing() override {
		if (!(RemoteState[24] & 0x0F))
			return ACOperand::SwingOff;

		return ACOperand::SwingAuto;
	}

	void SetFanMode(ACOperand::FanModeEnum Mode) override {
		  uint8_t fanset;

		  switch (Mode) {
		  	  case ACOperand::FanAuto:
		  		  fanset = 0b1010;
		  		  break;
		  	  case ACOperand::FanQuite:
		  		  fanset = 0b1011;
		  		  break;
		  	  default:
		  		  fanset = 2 + Mode;
		  }

		  RemoteState[24] &= 0x0F;
		  RemoteState[24] |= (fanset << 4);
	};

	ACOperand::FanModeEnum GetFanMode() override {
		  uint8_t fan = RemoteState[24] >> 4;

		  switch (fan) {
		  	  case 0b1011: return ACOperand::FanQuite	; break;
		  	  case 0b1010: return ACOperand::FanAuto	; break;
		  	  default:
		  		  if (fan > 7) fan = 7;

		  		  return static_cast<ACOperand::FanModeEnum>(fan-2);
		  }

		return ACOperand::FanAuto;
	};

	void Checksum() override {
		RemoteState[7] 	= Converter::SumBytes(RemoteState, Sections[0] - 1);
		RemoteState[15] = Converter::SumBytes(RemoteState + Sections[0], Sections[1] - 1);
		RemoteState[34] = Converter::SumBytes(RemoteState + Sections[0] + Sections[1], Sections[2] - 1);
	}
};

class Daikin2 : public DaikinUnit {
	public:
		Daikin2() : DaikinUnit() {
			DeviceType		= 2;

			StateLength 	= 39;
			Frequency 		= 36700;
			Gap				= 35204;

			HeaderMark		= 3500;
			HeaderSpace		= 1728;
			BitMark			= 460;
			OneSpace		= 1270;
			ZeroSpace		= 400;

			Sections		= {20, 19};

			Erase();
		}

		void Erase() override {
			for (uint8_t i = 0; i < StateLength; i++)
				RemoteState[i] = 0x0;

			RemoteState[0] = 0x11;
			RemoteState[1] = 0xDA;
			RemoteState[2] = 0x27;
			RemoteState[4] = 0x01;
			RemoteState[6] = 0xC0;
			RemoteState[7] = 0x70;
			RemoteState[8] = 0x08;
			RemoteState[9] = 0x0C;
			RemoteState[10] = 0x80;
			RemoteState[11] = 0x04;
			RemoteState[12] = 0xB0;
			RemoteState[13] = 0x16;
			RemoteState[14] = 0x24;
			RemoteState[17] = 0xBE;
			RemoteState[18] = 0xD0;
			// remote_state[19] is a checksum byte, it will be set by checksum().
			RemoteState[20] = 0x11;
			RemoteState[21] = 0xDA;
			RemoteState[22] = 0x27;
			RemoteState[25] = 0x08;
			RemoteState[28] = 0xA0;
			RemoteState[35] = 0xC1;
			RemoteState[36] = 0x80;
			RemoteState[37] = 0x60;
			// remote_state[38] is a checksum byte, it will be set by checksum().
			Checksum();
		}

		void SetMode(ACOperand::GenericMode Mode) override {
			uint8_t u8mode = ModeConverter(Mode);

			switch (Mode) {
				case ACOperand::ModeCool:
				case ACOperand::ModeHeat:
				case ACOperand::ModeFan:
				case ACOperand::ModeDry:
					break;
				default:
					u8mode = ModeConverter(ACOperand::ModeAuto);
			}

			if (Mode == ACOperand::ModeOff) {
				RemoteState[25] &= ~0b00000001;
				RemoteState[6] |= 0b10000000;
				return;
			}
			else {
				RemoteState[25] |= 0b00000001;
				RemoteState[6] &= ~0b10000000;
			}

			RemoteState[25] &= 0b10001111;
			RemoteState[25] |= (u8mode << 4);

			// Redo the temp setting as Cool mode has a different (std::min) temp.
			if (Mode == ACOperand::ModeCool)
				this->SetTemperature(this->GetTemperature());
		}

		ACOperand::GenericMode GetMode() override {
			if (!((RemoteState[25] & 0b00000001) && !(RemoteState[6] & 0b10000000)))
				return ACOperand::ModeOff;

			return ModeConverter(RemoteState[25] >> 4);
		}

		void SetTemperature(const uint8_t Temperature) override {
			uint8_t Temp = (GetMode() == ACOperand::GenericMode::ModeCool) ? 18 : 10;
			Temp = (std::max)(Temp, Temperature);
			Temp = (std::min)((uint8_t)32, Temp);
			RemoteState[26] = Temp * 2;
		}

		uint8_t GetTemperature() override {
			return RemoteState[26] / 2;
		}

		void SetHSwing(ACOperand::SwingMode Mode) override {
			uint8_t Position = 0;

			switch (Mode) {
				case ACOperand::SwingOff	: Position = 0x0	; break;
				case ACOperand::SwingBreeze	: Position = 0xBF	; break;
				default:
					Position  = 0xBE;
			}

			RemoteState[17] = Position;
		}

		ACOperand::SwingMode GetHSwing() override {
			return (RemoteState[17] == 0) ? ACOperand::SwingOff : ACOperand::SwingBreeze;
		}

		void SetVSwing(ACOperand::SwingMode Mode) override {
			uint8_t Position = 0;

			switch (Mode) {
				case ACOperand::SwingOff		: Position = 0x0; break;
				case ACOperand::SwingHigh		: Position = 0x1; break;
				case ACOperand::SwingLow		: Position = 0x6; break;
				case ACOperand::SwingBreeze		: Position = 0xC; break;
				case ACOperand::SwingCirculate	: Position = 0xD; break;
				case ACOperand::SwingAuto		:
					default						:
					Position = 0xE; break;
			}

			RemoteState[18] &= 0xF0;
			RemoteState[18] |= (Position & 0x0F);
		}

		ACOperand::SwingMode GetVSwing() override {
			uint8_t Position = RemoteState[18] & 0x0F;

			switch (Position) {
				case 0x1: return ACOperand::SwingHigh; 		break;
				case 0x6: return ACOperand::SwingLow; 		break;
				case 0xC: return ACOperand::SwingBreeze; 	break;
				case 0xD: return ACOperand::SwingCirculate; break;
				case 0xE: return ACOperand::SwingAuto; 		break;
				default:
					return ACOperand::SwingOff;
			}
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) override {
			  uint8_t fanset;

			  switch (Mode) {
			  	  case ACOperand::FanQuite	: fanset = 0b1011; break;
			  	  case ACOperand::FanAuto	: fanset = 0b1010; break;
			  	  default:
			  		  fanset = Mode+2;
			  }

			  RemoteState[28] &= 0x0F;
			  RemoteState[28] |= (fanset << 4);
		};

		ACOperand::FanModeEnum GetFanMode() override {
			  uint8_t fan = RemoteState[28] >> 4;

			  switch (fan) {
			  	  case 0b1011: return ACOperand::FanQuite	; break;
			  	  case 0b1010: return ACOperand::FanAuto	; break;
			  	  default:
			  		  if (fan > 7) fan = 7;

			  		  return static_cast<ACOperand::FanModeEnum>(fan-2);
			  }

			return ACOperand::FanAuto;
		};

		void Checksum() override {
			RemoteState[Sections[0] - 1] 	= Converter::SumBytes(RemoteState, Sections[0] - 1);
			RemoteState[StateLength -1 ]	= Converter::SumBytes(RemoteState + Sections[0], Sections[1] - 1);
		}
};

class Daikin128 : public DaikinUnit {
	public:
		Daikin128() : DaikinUnit() {
			DeviceType		= 128;

			StateLength 	= 16;
			Frequency 		= 38000;
			Gap				= 20300;

			HeaderMark		= 4600;
			HeaderSpace		= 2500;
			BitMark			= 350;
			OneSpace		= 954;
			ZeroSpace		= 382;

			Sections		= {8, 8};

			Erase();
		}

		void Erase() override {
			for (uint8_t i = 0; i < StateLength; i++)
				RemoteState[i] = 0x00;

			RemoteState[0] = 0x16;
			RemoteState[7] = 0x04;  // Most significant nibble is a checksum.
			RemoteState[8] = 0xA1;
			// RemoteState[15] is a checksum byte, it will be set by checksum().

			Checksum();
		}

		void SetMode(ACOperand::GenericMode Mode) override {
			uint8_t u8mode = 0;

			switch (Mode) {
				case ACOperand::ModeAuto	: u8mode = 0b00001010; break;
				case ACOperand::ModeCool	: u8mode = 0b00000010; break;
				case ACOperand::ModeHeat	: u8mode = 0b00001000; break;
				case ACOperand::ModeFan	: u8mode = 0b00000100; break;
				case ACOperand::ModeDry	: u8mode = 0b00000001; break;
				default:
					SetMode(ACOperand::ModeAuto);
			}

			RemoteState[1] &= ~0b00001111;
			RemoteState[1] |= u8mode;

			SetFanMode(GetFanMode());

			if (Mode == ACOperand::ModeOff)
				RemoteState[7] |= 0b00001000;
		}

		ACOperand::GenericMode GetMode() override {
			switch ((uint8_t)(RemoteState[1] & 0b00001111)) {
				case 0b00001010: return ACOperand::ModeAuto;
				case 0b00000010: return ACOperand::ModeCool;
				case 0b00001000: return ACOperand::ModeHeat;
				case 0b00000100: return ACOperand::ModeFan;
				case 0b00000001: return ACOperand::ModeDry;
				default:
					return ACOperand::ModeAuto;
			}
		}

		void SetTemperature(const uint8_t Temperature) override {
			RemoteState[6] = Converter::Uint8ToBcd((std::min)((uint8_t)30, (std::max)(Temperature, (uint8_t)16)));
		}

		uint8_t GetTemperature() override {
			return Converter::BcdToUint8(RemoteState[6]);
		}

		void SetHSwing(ACOperand::SwingMode Mode) override {
		}

		ACOperand::SwingMode GetHSwing() override {
			return ACOperand::SwingOff;
		}

		// ниже не готово
		void SetVSwing(ACOperand::SwingMode Mode) override {
			  if (ACOperand::SwingOff)
				  RemoteState[7] &= ~0b00000001;
			  else
				  RemoteState[7] |= 0b00000001;
		}

		ACOperand::SwingMode GetVSwing() override {
			if (RemoteState[7] & 0b00000001)
				return ACOperand::SwingAuto;
			else
				return ACOperand::SwingAuto;
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) override {
			uint8_t Speed = 0x0;

			switch (Mode) {
				case ACOperand::FanQuite:
				case ACOperand::FanAuto:
					Speed = (GetMode() == ACOperand::ModeAuto) ? 0b0001 : 0b1001;
					break;
				case ACOperand::Fan1:
				case ACOperand::Fan2:
					Speed = 0b1000;
					break;
				case ACOperand::Fan3:
					Speed = 0b0100;
					break;
				case ACOperand::Fan4:
					Speed = 0b0010;
					break;
				case ACOperand::Fan5:
					Speed = 0b0011;
					break;
			}

			RemoteState[1] &= ~0b11110000;
			RemoteState[1] |= (Speed << 4);
		};

		ACOperand::FanModeEnum GetFanMode() override {
			uint8_t FanSpeed = (RemoteState[1] & 0b11110000) >> 4;

			switch (FanSpeed) {
				case 0b0001: return ACOperand::FanAuto; break;
				case 0b1001: return ACOperand::FanQuite; break;
				case 0b1000: return ACOperand::Fan1; break;
				case 0b0100: return ACOperand::Fan3; break;
				case 0b0010: return ACOperand::Fan4; break;
				case 0b0011: return ACOperand::Fan5; break;
			}

			return ACOperand::FanAuto;
		};

		void Checksum() override {
			RemoteState[Sections[0] - 1] &= 0x0F;
			RemoteState[Sections[0] - 1] |= (CalcFirstChecksum(RemoteState) << 4);
			RemoteState[Sections[0] - 1] = CalcSecondChecksum(RemoteState);
		}

		uint8_t CalcFirstChecksum(const uint8_t state[]) {
		  return SumNibbles(state, Sections[0] - 1, state[Sections[0] - 1] & 0x0F) & 0x0F;
		}

		uint8_t CalcSecondChecksum(const uint8_t state[]) {
		  return SumNibbles(state + Sections[0], Sections[0] - 1);
		}

		uint8_t SumNibbles(const uint8_t * const start, const uint16_t length, const uint8_t init = 0) {
			uint8_t sum = init;
			const uint8_t *ptr;

		    for (ptr = start; ptr - start < length; ptr++)
		      sum += (*ptr >> 4) + (*ptr & 0xF);

		    return sum;
		}
};

class Daikin152 : public DaikinUnit {
	public:
		Daikin152() : DaikinUnit() {
			DeviceType		= 152;

			StateLength 	= 19;
			Frequency 		= 38000;
			Gap				= 25182;

			HeaderMark		= 3492;
			HeaderSpace		= 1718;
			BitMark			= 433;
			OneSpace		= 1529;
			ZeroSpace		= 433;

			Sections		= { 19 };

			Erase();
		}

		void Erase() override {
			for (uint8_t i = 3; i < StateLength; i++)
				RemoteState[i] = 0x00;

			RemoteState[0] =  0x11;
			RemoteState[1] =  0xDA;
			RemoteState[2] =  0x27;
			  // remote_state[19] is a checksum byte, it will be set by checksum().

			Checksum();
		}

		void SetMode(ACOperand::GenericMode Mode) override 		{ }

		ACOperand::GenericMode GetMode() override 				{ return ACOperand::ModeOff; }

		void SetTemperature(const uint8_t Temperature) override { }

		uint8_t GetTemperature() override 						{ return 0; }

		void SetHSwing(ACOperand::SwingMode Mode) override 		{ }

		ACOperand::SwingMode GetHSwing() override 				{ return ACOperand::SwingOff; }

		void SetVSwing(ACOperand::SwingMode Mode) override 		{ }

		ACOperand::SwingMode GetVSwing() override 				{ return ACOperand::SwingOff; }

		void SetFanMode(ACOperand::FanModeEnum Mode) override 	{ };

		ACOperand::FanModeEnum GetFanMode() override 			{ return ACOperand::FanAuto; };

		void Checksum() override 								{ };
};

class Daikin160 : public DaikinUnit {
	public:
		Daikin160() : DaikinUnit() {
			DeviceType		= 152;

			StateLength 	= 20;
			Frequency 		= 38000;
			Gap				= 29650;

			HeaderMark		= 5000;
			HeaderSpace		= 2145;
			BitMark			= 342;
			OneSpace		= 1786;
			ZeroSpace		= 700;

			Sections		= {7, 13};

			Erase();
		}

		void Erase() override {
			  for (uint8_t i = 0; i < StateLength; i++)
				  RemoteState[i] = 0x00;

			  RemoteState[0] =  0x11;
			  RemoteState[1] =  0xDA;
			  RemoteState[2] =  0x27;
			  RemoteState[3] =  0xF0;
			  RemoteState[4] =  0x0D;
			  // RemoteState[6] is a checksum byte, it will be set by checksum().
			  RemoteState[7] =  0x11;
			  RemoteState[8] =  0xDA;
			  RemoteState[9] =  0x27;
			  RemoteState[11] = 0xD3;
			  RemoteState[12] = 0x30;
			  RemoteState[13] = 0x11;
			  RemoteState[16] = 0x1E;
			  RemoteState[17] = 0x0A;
			  RemoteState[18] = 0x08;
			  // RemoteState[19] is a checksum byte, it will be set by checksum().

			  Checksum();
		}

		void SetMode(ACOperand::GenericMode Mode) override {
			uint8_t u8mode = 0x0;

			switch (Mode) {
		    	case ACOperand::ModeAuto: u8mode = 0b000; break;
		    	case ACOperand::ModeCool: u8mode = 0b011; break;
		    	case ACOperand::ModeHeat: u8mode = 0b100; break;
		    	case ACOperand::ModeFan	: u8mode = 0b110; break;
		    	case ACOperand::ModeDry	: u8mode = 0b010; break;
		    	default: break;
			}

			if (Mode == ACOperand::ModeOff)
				RemoteState[12] &= ~0b00000001;
			else
			{
				RemoteState[12] &= ~0b01110000;
				RemoteState[12] |= (u8mode << 4);
			}
		}

		ACOperand::GenericMode GetMode() override {
			if (!(RemoteState[12] & 0b00000001))
				return ACOperand::ModeOff;

			uint8_t u8mode = (RemoteState[12] & 0b01110000) >> 4;

			switch (u8mode) {
		    	case 0b000 : return ACOperand::ModeAuto	; break;
		    	case 0b011 : return ACOperand::ModeCool	; break;
		    	case 0b100 : return ACOperand::ModeHeat	; break;
		    	case 0b110 : return ACOperand::ModeFan	; break;
		    	case 0b010 : return ACOperand::ModeDry	; break;
			}

			return ACOperand::ModeAuto;
		}

		void SetTemperature(const uint8_t Temperature) override {
			uint8_t Degrees = (std::max)(Temperature, (uint8_t)10);
			Degrees = (std::min)(Degrees, (uint8_t)32) * 2 - 20;
			RemoteState[16] &= ~0b01111110;
			RemoteState[16] |= Degrees;
		}

		uint8_t GetTemperature() override {
			return (((RemoteState[16] & 0b01111110) / 2 ) + 10);
		}

		void SetHSwing(ACOperand::SwingMode Mode) override { }

		ACOperand::SwingMode GetHSwing() override {
			return ACOperand::SwingOff;
		}


		void SetVSwing(ACOperand::SwingMode Mode) override {
			uint8_t Position = 0;

			switch (Mode) {
				case ACOperand::SwingLow		: Position = 0x1; break;
				case ACOperand::SwingBreeze		: Position = 0x3; break;
				case ACOperand::SwingHigh		: Position = 0x5; break;
				default:
					Position = 0xF;
			}

			RemoteState[13] &= ~0b11110000;
			RemoteState[13] |= (Position << 4);
		}

		ACOperand::SwingMode GetVSwing() override {
			uint8_t Position = RemoteState[13] >> 4;

			switch (Position) {
				case 0x1: return ACOperand::SwingLow; 		break;
				case 0x3: return ACOperand::SwingBreeze;	break;
				case 0x5: return ACOperand::SwingHigh; 		break;
				default:
					return ACOperand::SwingAuto;
			}
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) override {
			uint8_t Speed = 0x0;

			switch (Mode) {
				case ACOperand::FanQuite	: Speed = 0b1011; break;
				case ACOperand::FanAuto		: Speed = 0b1010; break;
				case ACOperand::Fan1		:
				case ACOperand::Fan2		: Speed = 0b0011; break;
				case ACOperand::Fan3		: Speed = 0b0101; break;
				case ACOperand::Fan4		:
				case ACOperand::Fan5		: Speed = 0b0111; break;
				default:
					Speed = 0b1010;
			}

			RemoteState[17] &= ~0b00001111;
			RemoteState[17] |= Speed;
		};

		ACOperand::FanModeEnum GetFanMode() override {
			uint8_t FanSpeed = RemoteState[17] & 0b00001111;

			switch (FanSpeed) {
				case 0b1011: return ACOperand::FanQuite	; break;
				case 0b1010: return ACOperand::FanAuto	; break;
				case 0b0011: return ACOperand::Fan1		; break;
				case 0b0101: return ACOperand::Fan3		; break;
				case 0b0111: return ACOperand::Fan5		; break;
			}

			return ACOperand::FanAuto;
		};

		void Checksum() override {
			RemoteState[Sections[0] - 1] = Converter::SumBytes(RemoteState, Sections[0] - 1);
			RemoteState[StateLength - 1] = Converter::SumBytes(RemoteState + Sections[0], Sections[1] - 1);
		};
};

class Daikin176 : public DaikinUnit {
	public:
		Daikin176() : DaikinUnit() {
			DeviceType		= 176;

			StateLength 	= 22;
			Frequency 		= 38000;
			Gap				= 29410;

			HeaderMark		= 5070;
			HeaderSpace		= 2140;
			BitMark			= 370;
			OneSpace		= 1780;
			ZeroSpace		= 710;

			Sections		= {7, 15};

			Erase();
		}

		void Erase() override {
			  for (uint8_t i = 0; i < StateLength; i++)
				  RemoteState[i] = 0x00;

			  RemoteState[0] =  0x11;
			  RemoteState[1] =  0xDA;
			  RemoteState[2] =  0x17;
			  RemoteState[3] =  0x18;
			  RemoteState[4] =  0x04;
			  // RemoteState[6] is a checksum byte, it will be set by checksum().
			  RemoteState[7] =  0x11;
			  RemoteState[8] =  0xDA;
			  RemoteState[9] =  0x17;
			  RemoteState[10] = 0x18;
			  RemoteState[12] = 0x73;
			  RemoteState[14] = 0x20;
			  RemoteState[18] = 0x16;  // Fan speed and swing
			  RemoteState[20] = 0x20;
			  // RemoteState[21] is a checksum byte, it will be set by checksum().

			  Checksum();
		}

		void SetMode(ACOperand::GenericMode Mode) override {
			uint8_t u8mode = 0x0;
			uint8_t AltMode= 0x0;

			if (Mode == ACOperand::ModeOff) {
				RemoteState[13] = 0;
				RemoteState[14] &= ~0b00000001;
				return;
			}

			switch (Mode) {
    			case ACOperand::ModeFan:
    				u8mode = 0b110;
    				AltMode= 0;
    				break;

    			case ACOperand::ModeDry:
    				u8mode = 0b010;
    				AltMode= 7;
    				break;

	    		case ACOperand::ModeCool:
	    			u8mode = 0b111;
	    			AltMode= 2;
	    			break;

	    		default:
	    			SetMode(ACOperand::ModeCool);
	    			return;
			}

			uint8_t Temperature = GetTemperature();

			// Set the mode.
			RemoteState[12] &= ~0b01110000;
			RemoteState[12] |= (u8mode << 4);

			// Set the altmode
			RemoteState[12] &= ~0b01110000;
			RemoteState[12] |= (AltMode << 4);
			SetTemperature(Temperature);
			// Needs to happen after setTemp() as it will clear it.
			RemoteState[13] = 0b00000100;
		}

		ACOperand::GenericMode GetMode() override {
			if (!(RemoteState[14] & 0b00000001))
				return ACOperand::ModeOff;

			uint8_t u8mode =  (RemoteState[12] & 0b01110000) >> 4;

			switch (u8mode) {
	    		case 0b110 : return ACOperand::ModeFan	; break;
	    		case 0b010 : return ACOperand::ModeDry	; break;
	    		case 0b111 : return ACOperand::ModeCool	; break;
			}

			return ACOperand::ModeAuto;
		}

		void SetTemperature(const uint8_t Temperature) override {
			uint8_t Degrees = (std::min)((uint8_t)32, (std::max)(Temperature, (uint8_t)10));

			switch (GetMode()) {
				case ACOperand::ModeDry:
			    case ACOperand::ModeFan:
			    	Degrees = 17;
			    	break;
			    default: break;
			}

			Degrees = Degrees * 2 - 18;
			RemoteState[17] &= ~0b01111110;
			RemoteState[17] |= Degrees;
			RemoteState[13] = 0;
		}

		uint8_t GetTemperature() override {
			return (((RemoteState[17] & 0b01111110) / 2 ) + 9);
		}


		void SetHSwing(ACOperand::SwingMode Mode) override {
			uint8_t u8mode = 0x0;

			switch (Mode) {
				case ACOperand::SwingOff:
					u8mode = 0x6;
					break;
				case ACOperand::SwingAuto:
					u8mode = 0x5;
					break;
				default:
					SetHSwing(ACOperand::SwingAuto);
					return;
			}

			RemoteState[18] &= ~0b00001111;
			RemoteState[18] |= u8mode;
		}

		ACOperand::SwingMode GetHSwing() override {
			uint8_t u8Mode = RemoteState[18] & 0b00001111;

			switch (u8Mode) {
				case 0x6:
					return ACOperand::SwingOff;
					break;
				default:
					return ACOperand::SwingAuto;
					break;
			}
		}


		void SetVSwing(ACOperand::SwingMode Mode) override {}

		ACOperand::SwingMode GetVSwing() override {
			return ACOperand::SwingOff;
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) override {
			uint8_t Speed = 0x0;

			switch (Mode) {
				case ACOperand::Fan1	: Speed = 1; break;
				case ACOperand::Fan5	: Speed = 3; break;
				default:
					SetFanMode(ACOperand::Fan5);
					return;
			}

			RemoteState[18] &= ~0b11110000;
			RemoteState[18] |= (Speed << 4);
			RemoteState[13] = 0;
		};

		ACOperand::FanModeEnum GetFanMode() override {
			uint8_t FanSpeed = RemoteState[18] >> 4;

			switch (FanSpeed) {
				case 1:
					return ACOperand::Fan1;
					break;
				default:
					return ACOperand::Fan5;
			}
		};

		void Checksum() override {
			RemoteState[Sections[0] - 1] = Converter::SumBytes(RemoteState, Sections[0] - 1);
			RemoteState[StateLength - 1] = Converter::SumBytes(RemoteState + Sections[0], Sections[1] - 1);
		};
};

class Daikin216 : public DaikinUnit {
	public:
		Daikin216() : DaikinUnit() {
			DeviceType		= 216;

			StateLength 	= 27;
			Frequency 		= 38000;
			Gap				= 29650;

			HeaderMark		= 3440;
			HeaderSpace		= 1750;
			BitMark			= 420;
			OneSpace		= 1300;
			ZeroSpace		= 450;

			Sections		= {8, 19};

			Erase();
		}

		void Erase() override {
			for (uint8_t i = 0; i < StateLength; i++)
				RemoteState[i] = 0x00;

			RemoteState[0] =  0x11;
			RemoteState[1] =  0xDA;
			RemoteState[2] =  0x27;
			RemoteState[3] =  0xF0;

			// RemoteState[7] is a checksum byte, it will be set by checksum().
			RemoteState[8] =  0x11;
			RemoteState[9] =  0xDA;
			RemoteState[10] = 0x27;
			RemoteState[23] = 0xC0;
			// RemoteState[26] is a checksum byte, it will be set by checksum().
			Checksum();
		}

		void SetMode(ACOperand::GenericMode Mode) override {
			uint8_t u8mode = 0x0;

			if (Mode == ACOperand::ModeOff) {
				RemoteState[13] &= ~0b00000001;
				return;
			}

			switch (Mode) {
				case ACOperand::ModeAuto: u8mode = 0b000; break;
				case ACOperand::ModeCool: u8mode = 0b011; break;
				case ACOperand::ModeHeat: u8mode = 0b100; break;
				case ACOperand::ModeFan	: u8mode = 0b110; break;
				case ACOperand::ModeDry	: u8mode = 0b010; break;
				default: SetMode(ACOperand::ModeAuto); break;
			}

			RemoteState[13] &= ~0b01110000;
			RemoteState[13] |= (u8mode << 4);
		}

		ACOperand::GenericMode GetMode() override {
			if (!(RemoteState[13] & 0b00000001))
				return ACOperand::ModeOff;

			uint8_t u8mode = (RemoteState[13] & 0b01110000) >> 4;

			switch (u8mode) {
				case 0b000: return ACOperand::ModeAuto; 	break;
				case 0b011: return ACOperand::ModeCool; 	break;
				case 0b100: return ACOperand::ModeHeat; 	break;
				case 0b110: return ACOperand::ModeFan; 		break;
				case 0b010: return ACOperand::ModeDry; 		break;
			}

			return ACOperand::ModeAuto;
		}

		void SetTemperature(const uint8_t Temperature) override {
			uint8_t Degrees = (std::max)(Temperature, (uint8_t)10);
			Degrees = (std::min)(Degrees, (uint8_t)32);
			RemoteState[14] &= ~0b01111110;
			RemoteState[14] |= (Degrees << 1);
		}

		uint8_t GetTemperature() override {
			return (RemoteState[14] & 0b01111110) >> 1;
		}


		void SetHSwing(ACOperand::SwingMode Mode) override {
			if (Mode == ACOperand::SwingOff)
				RemoteState[17] &= ~0b00001111;
			else
				RemoteState[17] |= 0b00001111;
		}

		ACOperand::SwingMode GetHSwing() override {
			return (RemoteState[17] & 0b00001111) ? ACOperand::SwingAuto : ACOperand::SwingOff;

		}

		void SetVSwing(ACOperand::SwingMode Mode) override {
			if (Mode == ACOperand::SwingOff)
				RemoteState[16] &= ~0b00001111;
			else
				RemoteState[16] |= 0b00001111;
		}

		ACOperand::SwingMode GetVSwing() override {
			return (RemoteState[16] & 0b00001111) ? ACOperand::SwingAuto : ACOperand::SwingOff;
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) override {
			uint8_t Speed = 0x0;

			switch (Mode) {
				case ACOperand::FanAuto		: Speed = 0b1010; break;
				case ACOperand::FanQuite	: Speed = 0b1011; break;
				case ACOperand::Fan1		:
				case ACOperand::Fan2		: Speed = 0b0011; break;
				case ACOperand::Fan3		: Speed = 0b0101; break;
				case ACOperand::Fan4		:
				case ACOperand::Fan5		: Speed = 0b0111; break;
				default:
					SetFanMode(ACOperand::FanAuto);
					return;
			}

			RemoteState[16] &= ~0b11110000;
			RemoteState[16] |= (Speed << 4);
		};

		ACOperand::FanModeEnum GetFanMode() override {
			uint8_t FanSpeed = RemoteState[16] >> 4;

			switch (FanSpeed) {
				case 0b1010: return ACOperand::FanAuto	; break;
				case 0b1011: return ACOperand::FanQuite	; break;
				case 0b0011: return ACOperand::Fan1		; break;
				case 0b0101: return ACOperand::Fan3		; break;
				case 0b0111: return ACOperand::Fan5		; break;
				default:
					return ACOperand::FanAuto;
			}
		};

		void Checksum() override {
			RemoteState[Sections[0] - 1] = Converter::SumBytes(RemoteState, Sections[0] - 1);
			RemoteState[StateLength - 1] = Converter::SumBytes(RemoteState + Sections[0], Sections[1] - 1);
		};
};

class Daikin : public IRProto {
	private:
		vector<DaikinUnit*> Units;
	public:
		Daikin() {
			ID 					= 0x08;
			Name 				= "Daikin";
			DefinedFrequency	= 36700;

			if (Units.size() == 0)
				Units = {
					new DaikinGeneric(),
					new Daikin2(),
					new Daikin128(),
					new Daikin152(),
					new Daikin160(),
					new Daikin176(),
					new Daikin216()
				};
		};

		~Daikin() {
			for (auto& Unit : Units)
				if (Unit != nullptr)
					delete (Unit);
		}

		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() < 100)
				return false;

			for (auto &Unit: Units)
				if (Unit->IsProtocol(RawData))
					return true;

			return false;
		}

		DaikinUnit* GetUnit(uint8_t Code) {
			for (auto &Unit : Units)
				if (Unit->DeviceType == Code)
					return Unit;

			return nullptr;
		}

		DaikinUnit* GetUnit(vector<int32_t> &Raw) {
			for (auto &Unit : Units)
				if (Unit->IsProtocol(Raw))
					return Unit;

			return nullptr;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) override {
			ACOperand AC(Data);

			DaikinUnit* Unit = GetUnit(AC.DeviceType);

			if (Unit == nullptr)
				return vector<int32_t>();

			Unit->SetTemperature(AC.Temperature);
			Unit->SetHSwing(AC.HSwingMode);
			Unit->SetVSwing(AC.VSwingMode);

			Unit->SetMode(AC.Mode);

			DefinedFrequency = Unit->Frequency;

			return Unit->GetSignal();
		}

		pair<uint32_t,uint16_t>	GetData(vector<int32_t> Raw) override {
			DaikinUnit* Unit = GetUnit(Raw);

			if (Unit == nullptr)
				return make_pair(0x0,0x0);

			return make_pair(Unit->GetOperand(Raw).ToUint32(), 0x0);
		}

		int32_t GetBlocksDelimeter() override {
			return -36000;
		}
};
