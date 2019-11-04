/*
 *    MitsubishiAC.cpp
 *    Class for MitsubishiAC protocol
 *    136 or 144 state length
 *
 *
 */

#include "DateTime.h"

enum GreeACType { GreeNULL = 0, Gree1 = 1, Gree2 = 2};
// Gree1: Ultimate, EKOKAI, RusClimate (Default)
// Gree2: Green, YBOFB2, YAPOF3

class Gree : public IRProto {
	public:
		GreeACType 	Type 			= GreeNULL;

		uint8_t		StateLength 	= 8;
		uint16_t	Gap				= 19000;

		uint16_t	HeaderMark		= 9000;
		uint16_t	HeaderSpace		= 4500;
		uint16_t	BitMark			= 620;
		uint16_t	OneSpace		= 1600;
		uint16_t	ZeroSpace		= 540;

		uint8_t 	RemoteState[18];


		Gree() {
			ID 					= 0x0A;
			Name 				= "Gree";
			DefinedFrequency	= 38000;
		};


		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() != 140)
				return false;

			if (TestValue(RawData.at(0), HeaderMark) &&
				TestValue(RawData.at(1), -HeaderSpace) &&
				TestValue(RawData.at(2), BitMark) &&
				TestValue(RawData.at(74), -Gap) &&
				RawData.size() == 140)
				return true;

			return false;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) override {
			ACOperand AC(Data);

			Type = static_cast<GreeACType>(AC.DeviceType);

			Erase();

			SetMode(AC.Mode);
			SetTemperature(AC.Temperature);
			SetHSwing(AC.HSwingMode);
			SetVSwing(AC.VSwingMode);
			SetFanMode(AC.FanMode);

			SetPower(GetPower());
			Checksum();

			vector<int32_t> Result;

			Result.push_back(HeaderMark);
			Result.push_back(-HeaderSpace);

			for (int i = 0; i < 4; i++) {
				//ESP_LOGE("RemoteState", "[%d], %02X", i, RemoteState[i]);
				bitset<8> Byte(RemoteState[i]);
				for (int j = 0; j < 8; j++) {
					Result.push_back(BitMark);
					Result.push_back((Byte[j] == true) ? -OneSpace : -ZeroSpace);
				}
			}

			Result.push_back(BitMark); Result.push_back(-ZeroSpace);
			Result.push_back(BitMark); Result.push_back(-OneSpace);
			Result.push_back(BitMark); Result.push_back(-ZeroSpace);
			Result.push_back(BitMark); Result.push_back(-Gap);

			for (int i = 4; i < StateLength; i++) {
				//ESP_LOGE("RemoteState", "[%d], %02X", i, RemoteState[i]);
				bitset<8> Byte(RemoteState[i]);
				for (int j = 0; j < 8; j++) {
					Result.push_back(BitMark);
					Result.push_back((Byte[j] == true) ? -OneSpace : -ZeroSpace);
				}
			}

			Result.push_back(BitMark);
			Result.push_back(-Settings.SensorsConfig.IR.SignalEndingLen);

			return Result;
		}

		pair<uint32_t,uint16_t>	GetData(vector<int32_t> Raw) override {
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

				if (i == 3)
					for (int k = 0; k < 8; k++)
						Raw.erase(Raw.begin());
			}

			ACOperand AC;

			AC.DeviceType 	= GetType();
			AC.Mode			= GetMode();
			AC.Temperature	= GetTemperature();
			AC.HSwingMode	= GetHSwing();
			AC.VSwingMode	= GetVSwing();
			AC.FanMode 		= GetFanMode();

			return make_pair(AC.ToUint32(), 0x0);
		}

		int32_t GetBlocksDelimeter() override {
			return -19000;
		}

		void Erase() {
			for (uint8_t i = 0; i < StateLength; i++)
				RemoteState[i] = 0x0;

			RemoteState[1] = 0x09;
			RemoteState[2] = 0x20;
			RemoteState[3] = 0x50;
			RemoteState[5] = 0x20;
			RemoteState[7] = 0x50;

			Checksum();
		}

		GreeACType GetType() {
			if (GetPower())
				Type = (RemoteState[2] & 0b01000000) ? Gree1 : Gree2;

			return GreeNULL;
		}

		void SetPower(bool Power) {
			if (Power) {
				RemoteState[0] |= 0b00001000;

				switch (Type) {
			      case GreeACType::Gree2: break;
			      default:
			    	  RemoteState[2] |= 0b01000000;
			    }
			}
			else {
				RemoteState[0] &= ~0b00001000;
				RemoteState[2] &= ~0b01000000;
			}
		}

		bool GetPower() {
			return (RemoteState[0] & 0b00001000) ? true : false;
		}

		void SetMode(ACOperand::GenericMode Mode) {
			uint8_t u8mode = 0x0;

			switch (Mode) {
				case ACOperand::ModeAuto	:
					u8mode = 0;
					SetTemperature(25);
					break;
				case ACOperand::ModeCool	: u8mode = 1; break;
				case ACOperand::ModeDry		:
					u8mode = 2;
					SetFanMode(ACOperand::Fan1);
					break;
				case ACOperand::ModeFan		: u8mode = 3; break;
				case ACOperand::ModeHeat	: u8mode = 4; break;
				default:
					u8mode = 0;
			}

			SetPower((Mode != ACOperand::ModeOff));

			RemoteState[0] &= ~0b00000111;
			RemoteState[0] |= u8mode;
		}

		ACOperand::GenericMode GetMode() {
			if (!GetPower())
				return ACOperand::ModeOff;

			uint8_t u8mode = (RemoteState[0] & 0b00000111);

			switch (u8mode) {
				case 0: return ACOperand::ModeAuto	; break;
				case 1: return ACOperand::ModeCool	; break;
				case 2: return ACOperand::ModeDry	; break;
				case 3: return ACOperand::ModeFan	; break;
				case 4: return ACOperand::ModeHeat	; break;
				default:
					return ACOperand::ModeAuto;
			}
		}

		void SetTemperature(const uint8_t Temperature) {
			  uint8_t Temp = max((uint8_t)16, Temperature);
			  Temp = min((uint8_t)30, Temp);

			  if (GetMode() == ACOperand::ModeAuto)
				  Temp = 25;

			  RemoteState[1] = (RemoteState[1] & ~0b00001111) | (Temp - 16);
		}

		uint8_t GetTemperature() {
			return ((RemoteState[1] & 0b00001111) + 16);
		}

		void SetHSwing(ACOperand::SwingMode Mode) {}

		ACOperand::SwingMode GetHSwing() {
			return ACOperand::SwingOff;
		}

		void SetVSwing(ACOperand::SwingMode Mode) {
			RemoteState[0] &= ~0b01000000;

			if (Mode == ACOperand::SwingAuto) {
				RemoteState[0] |= 0b01000000;
				RemoteState[4] &= ~0b00001111;
				RemoteState[4] |= 0b00000001;
			}
			else {
				RemoteState[4] &= ~0b00001111;
				RemoteState[4] |= 0b00000000;
			}
		}

		ACOperand::SwingMode GetVSwing() {
			return (RemoteState[0] & 0b01000000) ? ACOperand::SwingAuto : ACOperand::SwingOff;
		}

		void SetFanMode(ACOperand::FanModeEnum Mode) {
			uint8_t u8mode = 0x0;

			switch (Mode) {
				case ACOperand::FanAuto:
					u8mode = 0;
					break;
				case ACOperand::FanQuite:
				case ACOperand::Fan1:
					u8mode = 1;
					break;
				case ACOperand::Fan2:
				case ACOperand::Fan3:
					u8mode = 2;
					break;
				case ACOperand::Fan4:
				case ACOperand::Fan5:
					u8mode = 3;
					break;
				default:
					SetFanMode(ACOperand::FanAuto);
			}

			if (GetMode() == ACOperand::ModeDry)
				u8mode = 1;

			RemoteState[0] &= ~0b00110000;
			RemoteState[0] |= (u8mode << 4);
		};

		ACOperand::FanModeEnum GetFanMode() {
			uint8_t u8mode = (RemoteState[0] & 0b00110000) >> 4;;

			switch (u8mode) {
				case 1: return ACOperand::Fan1; 	break;
				case 2: return ACOperand::Fan3; 	break;
				case 3: return ACOperand::Fan5; 	break;
				default:return ACOperand::FanAuto;	break;
			}
		};

		void Checksum() {
			uint8_t Sum = 10;
			for (uint8_t i = 0; i < 4 && i < StateLength - 1; i++)
				Sum += (RemoteState[i] & 0x0FU);

			// then sum the upper half of the next 3 bytes.
			for (uint8_t i = 4; i < StateLength - 1; i++)
				Sum += (RemoteState[i] >> 4);

			RemoteState[StateLength - 1] = (Sum & 0x0FU << 4) | (RemoteState[StateLength - 1] & 0xFU);
		}
};
