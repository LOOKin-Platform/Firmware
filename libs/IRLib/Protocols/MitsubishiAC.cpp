/*
 *    MitsubishiAC.cpp
 *    Class for MitsubishiAC protocol
 *    136 or 144 state length
 *
 *
 */

#include "DateTime.h"

enum MitsubishiACType { MACNULL = 0, MAC136 = 136, MAC144 = 144 };

class MitsubishiAC : public IRProto {
	public:
		MitsubishiACType Type 		= MitsubishiACType::MACNULL;

		uint8_t		StateLength 	= 0;
		uint16_t	Gap				= 0;

		uint16_t	HeaderMark		= 0;
		uint16_t	HeaderSpace		= 0;
		uint16_t	BitMark			= 0;
		uint16_t	OneSpace		= 0;
		uint16_t	ZeroSpace		= 0;

		uint8_t 	RemoteState[18];


		MitsubishiAC() {
			ID 					= 0x09;
			Name 				= "Mitsubishi-AC";
			DefinedFrequency	= 38000;
		};

		void FillProtocolData(MitsubishiACType ACType = MitsubishiACType::MACNULL) {
			if (ACType != MitsubishiACType::MACNULL)
				Type = ACType;

			if (Type == MAC136) {
				StateLength 	= 17;
				Gap				= 36000;

				HeaderMark		= 3324;
				HeaderSpace		= 1474;
				BitMark			= 467;
				OneSpace		= 1137;
				ZeroSpace		= 351;
			}
			else if (Type == MAC144) {
				StateLength 	= 18;
				Gap				= 36000;

				HeaderMark		= 3400;
				HeaderSpace		= 1750;
				BitMark			= 450;
				OneSpace		= 1300;
				ZeroSpace		= 420;
			}
		}

		bool FillProtocolData(vector<int32_t> &Raw) {
			if (CheckType(MitsubishiACType::MAC136, Raw))
				return true;

			if (CheckType(MitsubishiACType::MAC144, Raw))
				return true;

			return false;
		}

		bool CheckType(MitsubishiACType ACType, vector<int32_t> &Raw) {
			FillProtocolData(ACType);

			int16_t Diff = StateLength*16 + 4 - Raw.size();

			if (abs(Diff) < 3)
				if (TestValue(Raw.front(), HeaderMark) && TestValue(Raw.at(1), -HeaderSpace) && TestValue(Raw.at(2), BitMark))
					return true;

			return false;
		}

		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() < 274)
				return false;

			if (CheckType(MitsubishiACType::MAC136, RawData))
				return true;

			if (CheckType(MitsubishiACType::MAC144, RawData))
				return true;

			return false;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) override {
			ACOperand AC(Data);

			Type = static_cast<MitsubishiACType>(AC.DeviceType);
			FillProtocolData();

			Erase();

			SetMode(AC.Mode);
			SetTemperature(AC.Temperature);
			SetHSwing(AC.HSwingMode);
			SetVSwing(AC.VSwingMode);
			SetFanMode(AC.FanMode);
			SetTime(Time::DateTime());

			Checksum();

			vector<int32_t> Result;

			Result.push_back(HeaderMark);
			Result.push_back(-HeaderSpace);

			for (int i = 0; i < StateLength; i++) {
				//ESP_LOGE("RemoteState", "[%d], %02X", i, RemoteState[i]);
				bitset<8> Byte(RemoteState[i]);
				for (int j = 0; j < 8; j++) {
					Result.push_back(BitMark);
					Result.push_back((Byte[j] == true) ? -OneSpace : -ZeroSpace);
				}
			}

			if (Type == MitsubishiACType::MAC144)
			{
				Result.push_back(BitMark);
				Result.push_back(-20000);
				Result.insert(Result.end(), Result.begin(), Result.end()-2);
			}

			Result.push_back(BitMark);
			Result.push_back(-Settings.SensorsConfig.IR.SignalEndingLen);

			return Result;
		}

		pair<uint32_t,uint16_t>	GetData(vector<int32_t> Raw) override {
			FillProtocolData(Raw);

			Erase();

			Raw.erase(Raw.begin()); Raw.erase(Raw.begin());

			for (int i = 0; i < StateLength; i++)
			{
				bitset<8> Byte;
				for (int j = 0; j < 8; j++)
				{
					if (!IRProto::TestValue(Raw.front(), BitMark))
						return make_pair(0x0, 0x0);

				    Raw.erase(Raw.begin());

					if      (IRProto::TestValue(Raw.front(), -OneSpace)) 	Byte[j] = 1;
					else if	(IRProto::TestValue(Raw.front(), -ZeroSpace))	Byte[j] = 0;
					else
						return make_pair(0x0, 0x0);

			    	Raw.erase(Raw.begin());
				}

				RemoteState[i] = (uint8_t)(Byte.to_ulong());
			}

			/*
			for (int i=0; i< StateLength; i++)
				ESP_LOGE("RemoteState", "[%d] = %s", i, Converter::ToHexString(RemoteState[i],2).c_str());
			*/

			ACOperand AC;

			AC.DeviceType 	= static_cast<uint8_t>(Type);
			AC.Mode			= GetMode();
			AC.Temperature	= GetTemperature();
			AC.HSwingMode	= GetHSwing();
			AC.VSwingMode	= GetVSwing();
			AC.FanMode 		= GetFanMode();

			return make_pair(AC.ToUint32(), 0x0);
		}

		int32_t GetBlocksDelimeter() override {
			return -36000;
		}

		void Erase() {
			if (Type == MitsubishiACType::MAC136) {
				RemoteState[0] = 0x23;
				RemoteState[1] = 0xCB;
				RemoteState[2] = 0x26;
				RemoteState[3] = 0x21;
				RemoteState[4] = 0x00;
				RemoteState[5] = 0x40;
				RemoteState[6] = 0xC2;
				RemoteState[7] = 0xC7;
				RemoteState[8] = 0x04;
				RemoteState[9] = 0x00;
				RemoteState[10] = 0x00;
				Checksum();  // Calculate the checksum which cove
			}

			if (Type == MitsubishiACType::MAC144) {
				RemoteState[0] = 0x23;
				RemoteState[1] = 0xCB;
				RemoteState[2] = 0x26;
				RemoteState[3] = 0x01;
				RemoteState[4] = 0x00;
				RemoteState[5] = 0x20;
				RemoteState[6] = 0x08;
				RemoteState[7] = 0x06;
				RemoteState[8] = 0x30;
				RemoteState[9] = 0x45;
				RemoteState[10] = 0x67;

				for (uint8_t i = 11; i < StateLength - 1; i++)
					RemoteState[i] = 0;

				RemoteState[StateLength - 1] = 0x1F;

				Checksum();
			}

		}

		void SetMode(ACOperand::GenericMode Mode) {
			uint8_t u8mode = 0x0;

			if (Type == MitsubishiACType::MAC136) {
				switch (Mode) {
					case ACOperand::ModeCool	: u8mode = 0b001; break;
					case ACOperand::ModeHeat	: u8mode = 0b010; break;
					case ACOperand::ModeFan		: u8mode = 0b000; break;
					case ACOperand::ModeAuto	: u8mode = 0b011; break;
					case ACOperand::ModeDry		: u8mode = 0b101; break;
					default:
						u8mode = 0b011;
				}

				if (Mode == ACOperand::ModeOff)
					RemoteState[5] &= ~0b01000000;
				else
					RemoteState[5] |= 0b01000000;

				RemoteState[6] &= ~0b00000111;
				RemoteState[6] |= u8mode;
			}

			if (Type == MitsubishiACType::MAC144) {
				switch (Mode) {
					case ACOperand::ModeCool:
						u8mode = 0x18;
						RemoteState[8] = 0b00000110; //0b00110110;
						break;

					case ACOperand::ModeHeat:
						u8mode = 0x08;
						RemoteState[8] = 0b00000000; //0b00110000;

						break;
					case ACOperand::ModeAuto:
						u8mode = 0x20;
						RemoteState[8] = 0b00000000; // 0b00110000;
						break;

					case ACOperand::ModeDry:
						u8mode = 0x10;
						RemoteState[8] = 0b00000010; // 0b00110010;
						break;

					case ACOperand::ModeOff:
						u8mode = 0x20;
						RemoteState[8] = 0b00000000; // 0b00110000
						break;

					default:
						SetMode(ACOperand::ModeAuto);
						return;
				}

				if (Mode == ACOperand::ModeOff)
					RemoteState[5] &= ~0x20;
				else
					RemoteState[5] |= 0x20;

				RemoteState[6] = u8mode;
			}
		}

		ACOperand::GenericMode GetMode() {
			if (Type == MitsubishiACType::MAC136) {
				if (!(RemoteState[5] & 0b01000000))
					return ACOperand::ModeOff;

				uint8_t u8mode = RemoteState[6] & 0b00000111;
				switch (u8mode) {
					case 0b000: return ACOperand::ModeFan	; break;
					case 0b001: return ACOperand::ModeCool	; break;
					case 0b010: return ACOperand::ModeHeat	; break;
					case 0b011: return ACOperand::ModeAuto	; break;
					case 0b101: return ACOperand::ModeDry	; break;
				}
			}

			if (Type == MitsubishiACType::MAC144) {
				if  (!((RemoteState[5] & 0x20) != 0))
					return ACOperand::ModeOff;

				uint8_t u8mode = RemoteState[6];

				switch (u8mode) {
					case 0x18: return ACOperand::ModeCool	; break;
					case 0x08: return ACOperand::ModeHeat	; break;
					case 0x20: return ACOperand::ModeAuto	; break;
					case 0x10: return ACOperand::ModeDry	; break;
				}
			}

			return ACOperand::ModeAuto;
		}

		void SetTemperature(const uint8_t Temperature) {
			if (Type == MitsubishiACType::MAC136) {
				uint8_t Temp = (std::max)((uint8_t)17, Temperature);
				Temp = (std::min)((uint8_t)30, Temp);
				RemoteState[6] &= ~0b11110000;
				RemoteState[6] |= ((Temp - 17) << 4);
			}

			if (Type == MitsubishiACType::MAC144) {
				uint8_t Temp = (std::max)((uint8_t)16, Temperature);
				Temp = (std::min)((uint8_t)31, Temp);
				RemoteState[7] = Temp - 16;
			}
		}

		uint8_t GetTemperature() {
			if (Type == MitsubishiACType::MAC136)
				return (RemoteState[6] >> 4) + 16;

			if (Type == MitsubishiACType::MAC144)
			{
				uint8_t Temperature = RemoteState[7];
				Temperature &= 0b00001111;
				Temperature+= 16;

				if (Temperature > 31) return 31;
				if (Temperature < 16) return 16;
				return Temperature;
			}

			return 0;
		}

		void SetHSwing(ACOperand::SwingMode Mode) {
		}

		ACOperand::SwingMode GetHSwing() {
			return ACOperand::SwingOff;
		}

		void SetVSwing(ACOperand::SwingMode Mode) {
			uint8_t u8mode = 0x0;

			if (Type == MitsubishiACType::MAC136) {
				switch (Mode) {
					case ACOperand::SwingLow	: u8mode = 0b0000; break;
					case ACOperand::SwingBreeze	: u8mode = 0b0001; break;
					case ACOperand::SwingHigh	: u8mode = 0b0010; break;
					case ACOperand::SwingAuto	: u8mode = 0b1100; break;
					default:
						SetVSwing(ACOperand::SwingAuto);
						return;
				}

				RemoteState[7] &= ~0b11110000;
				RemoteState[7] |= u8mode << 4;
			}

			if (Type == MitsubishiACType::MAC144) {
				u8mode = 0x0;

				switch (Mode)
				{
					case ACOperand::SwingBottom:
						u8mode = 0b01001000;
						break;
					case ACOperand::SwingMiddle:
						u8mode = 0b01011000;
						break;
					case ACOperand::SwingTop:
						u8mode = 0b01101000;
						break;
					default:
						u8mode = 0b01111000;
						break;
				}
				RemoteState[9] &= 0b10000111;  // Clear the previous setting.
				RemoteState[9] |= u8mode;
			}
		}

		ACOperand::SwingMode GetVSwing() {

			if (Type == MitsubishiACType::MAC136) {
				uint8_t u8mode = 0x0;

				u8mode = (RemoteState[7] & 0b11110000) >> 4;

				switch (u8mode) {
					case 0b0000: return ACOperand::SwingLow; break;
					case 0b0001: return ACOperand::SwingBreeze; break;
					case 0b0010: return ACOperand::SwingHigh; break;
					case 0b1100: return ACOperand::SwingAuto; break;
					default:
						return ACOperand::SwingAuto;
				}
			}

			if (Type == MitsubishiACType::MAC144) {
				uint8_t u8mode = RemoteState[9] & 0b01111000;

				switch (u8mode) {
					case 0b01001000: return ACOperand::SwingBottom;
					case 0b01011000: return ACOperand::SwingMiddle;
					case 0b01101000: return ACOperand::SwingTop;
					default:
						return ACOperand::SwingAuto;
				}
			}

			return ACOperand::SwingOff;
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) {
			uint8_t u8mode = 0x0;

			if (Type == MitsubishiACType::MAC136) {
				switch (Mode) {
					case ACOperand::FanQuite	: u8mode = 0b00; break;
					case ACOperand::Fan1		: u8mode = 0b01; break;
					case ACOperand::Fan3		: u8mode = 0b10; break;
					case ACOperand::Fan5		: u8mode = 0b11; break;
					default:
						SetFanMode(ACOperand::Fan3);
						return;
				}

				RemoteState[7] &= ~0b00000110;
				RemoteState[7] |= (u8mode << 1);
			}

			if (Type == MitsubishiACType::MAC144) {
				switch (Mode) {
					case ACOperand::FanQuite	:
					case ACOperand::Fan1		:
					case ACOperand::Fan2		:
						u8mode = 0b00000001;
						break;
					case ACOperand::Fan3		:
						u8mode = 0b00000010;
						break;
					case ACOperand::Fan4		:
					case ACOperand::Fan5		:
						u8mode = 0b00000011;
						break;
					case ACOperand::FanAuto		:
						u8mode = 0b00000000;
						//RemoteState[9] = 0b10000000 | (RemoteState[9] & 0b01111000);
						break;
				}

				RemoteState[9] &= 0b01111000;  // Clear the previous state
				RemoteState[9] |= u8mode;
			}
		};

		ACOperand::FanModeEnum GetFanMode() {
			uint8_t u8mode = 0x0;

			if (Type == MitsubishiACType::MAC136) {
				u8mode = (RemoteState[7] & 0b00000110) >> 1;

				switch (u8mode) {
					case 0b00: return ACOperand::FanQuite	; break;
					case 0b01: return ACOperand::Fan1		; break;
					case 0b10: return ACOperand::Fan3		; break;
					case 0b11: return ACOperand::Fan5		; break;
					default:
						return ACOperand::Fan3;
				}
			}

			if (Type == MitsubishiACType::MAC144) {
				u8mode = RemoteState[9] & 0b111;

				switch (u8mode) {
					case 0b00000001: return ACOperand::Fan1;
					case 0b00000010: return ACOperand::Fan3;
					case 0b00000011: return ACOperand::Fan5;
					default:
						return ACOperand::FanAuto;
					//case 0x6: return ACOperand::FanQuite	; break;
					//case 0x4: return ACOperand::Fan5		; break;
				}
			}

			return ACOperand::FanAuto;
		};

		void SetTime(DateTime_t DateTime) {
			ESP_LOGE("DateTime", "Hours: %d, Minutes: %d", DateTime.Hours, DateTime.Minutes);

			if (Type == MitsubishiACType::MAC144)
				RemoteState[10] = DateTime.Hours * 6 + (uint8_t)(DateTime.Minutes / 10);
		}

		DateTime_t GetTime() {
			DateTime_t Result;

			if (Type == MitsubishiACType::MAC144)
			{
				Result.Hours 	= (uint8_t)(RemoteState[10] / 6);
				Result.Minutes	= ((uint8_t)(RemoteState[10] % 6)) * 10;
			}

			return DateTime_t();
		}

		void Checksum() {
			if (Type == MitsubishiACType::MAC136) {
				for (uint8_t i = 0; i < 6; i++)
					RemoteState[5 + 6 + i] = ~RemoteState[5 + i];
			}

			if (Type == MitsubishiACType::MAC144) {
				uint8_t CheckSum = 0;
				for (uint8_t i = 0; i < 17; i++)
					CheckSum += RemoteState[i];

				CheckSum = CheckSum & 0xFFU;

				RemoteState[17] = CheckSum;
			}
		}
};
