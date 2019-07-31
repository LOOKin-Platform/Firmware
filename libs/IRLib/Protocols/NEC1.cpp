/*
 *    NEC1.cpp
 *    Class for NEC1 protocol
 *
 */

class NEC1 : public IRProto {
	public:
		NEC1() {
			ID 					= 0x01;
			Name 				= "NEC1";
			DefinedFrequency	= 38500;
		};

		bool IsProtocol(vector<int32_t> RawData) override {
			if (RawData.size() >= 66) {
				if (RawData.at(0) 	> 8700 	&& RawData.at(0) 	< 9300 &&
					RawData.at(1) 	< -4200 && RawData.at(1) 	> -4800 &&
					RawData.at(66) 	> 500 	&& RawData.at(66)	< 700)
					return true;
			}

			if (RawData.size() == 16) {
				if (RawData.at(0) > 8700 && RawData.at(0) < 9300 &&
					RawData.at(1) < -1900 && RawData.at(1) > -2500 &&
					RawData.at(2) > 470 && RawData.at(2) < 700)
				return true;
			}

			return false;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {
			uint16_t 	Address = 0x0;
			uint8_t		Command = 0x0;

			vector<int32_t> Data = RawData;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

		    if (Data.size() == 16) { // repeat signal
				return make_pair(0xFFFFFFFF,0x0);
		    }

		    for (uint8_t BlockId = 0; BlockId < 4; BlockId++) {
		    		bitset<8> Block;

				for (int i=0; i<8; i++) {
					if (Data[0] > 500 && Data[0] < 720) {
						if (Data[1] > -700)
							Block[i] = 0;
						if (Data[1] < -1200)
							Block[i] = 1;
					}

					Data.erase(Data.begin()); Data.erase(Data.begin());
				}

				uint8_t BlockInt = (uint8_t)Block.to_ulong();

				switch (BlockId) {
					case 0: Address = BlockInt; break;
					case 1:
						if ((uint8_t)Address != (uint8_t)~BlockInt) {
							uint16_t tmpAddress = BlockInt;
							tmpAddress = tmpAddress << 8;
							Address += tmpAddress;
						}
						break;
					case 2: Command = BlockInt; break;
					case 3:
						if (Command != (uint8_t)~BlockInt) Command = 0;
						break;
				}
		    }

		    return make_pair((Address << 8) + Command, 0x0);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			uint16_t	 NECAddress = (uint16_t)((Data >> 8));
			uint8_t 	 NECCommand = (uint8_t) ((Data << 24) >> 24);

			vector<int32_t> Raw = vector<int32_t>();

			/* raw NEC header */
			Raw.push_back(+9000);
			Raw.push_back(-4500);

			if (NECAddress <= 0xFF)
				NECAddress += (((uint8_t)~NECAddress) << 8);

			bitset<16> AddressItem(NECAddress);
			for (int i=0;i<16;i++) {
				Raw.push_back(+560);
				Raw.push_back((AddressItem[i] == true) ? -1650 : -560);
			}

			bitset<8> CommandItem(NECCommand);
			for (int i=0;i<8;i++) {
				Raw.push_back(+560);
				Raw.push_back((CommandItem[i] == true) ? -1650 : -560);
			}

			for (int i=0;i<8;i++) {
				Raw.push_back(+560);
				Raw.push_back((CommandItem[i] == false) ? -1650 : -560);
			}

			Raw.push_back(+560);
			Raw.push_back(-1650);
			Raw.push_back(-1650);
			Raw.push_back(+8900);
			Raw.push_back(-2250);
			Raw.push_back(+560);

			return Raw;
		}

		vector<int32_t> ConstructRawRepeatSignal(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Result = vector<int32_t>();

			Result.push_back(+9000);
			Result.push_back(-2250);
			Result.push_back(+560);
			Result.push_back(-45000);

			return Result;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) {
			return ConstructRaw(Data, Misc);
		}

};
