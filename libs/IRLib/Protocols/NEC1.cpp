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

		bool IsProtocol(vector<int32_t> &RawData, uint16_t Start, uint16_t Length) override {
			if (Length == 68) {
				if (RawData.at(Start + 0) 	> 7650 	&& RawData.at(Start + 0) < 10350 &&
					RawData.at(Start + 1) 	< -3825 && RawData.at(Start + 1) 	> -5200 &&
					RawData.at(Start + 66) 	> 450 	&& RawData.at(Start + 66)	< 700)
					return true;
			}

			if (Length == 16) {
				if (RawData.at(Start + 0) > 7650 && RawData.at(Start + 0) < 10350 &&
					RawData.at(Start + 1) < -1900 && RawData.at(Start + 1) > -2500 &&
					RawData.at(Start + 2) > 450 && RawData.at(Start + 2) < 700)
				return true;
			}

			return false;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {
			uint32_t 	Uint32Data = 0x0;

			vector<int32_t> Data = RawData;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

		    if (Data.size() == 16) { // repeat signal
				return make_pair(0xFFFFFFFF,0x0);
		    }

		    for (uint8_t BlockId = 0; BlockId < 4; BlockId++) {
		    	bitset<8> Block;

				for (int i=7; i>=0; i--) {
					if (Data[0] > 450 && Data[0] < 720) {
						if (Data[1] > -700)
							Block[i] = 0;
						if (Data[1] < -1200)
							Block[i] = 1;
					}

					Data.erase(Data.begin()); Data.erase(Data.begin());
				}

				uint8_t BlockInt = (uint8_t)Block.to_ulong();

				Uint32Data = (Uint32Data << 8) + BlockInt;
		    }

		    return make_pair(Uint32Data, 0x0);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			/* raw NEC header */
			Raw.push_back(+9000);
			Raw.push_back(-4500);

			bitset<32> AddressItem(Data);

			for (int i=31;i>=0;i--) {
				Raw.push_back(+560);
				Raw.push_back((AddressItem[i] == true) ? -1650 : -560);
			}

			Raw.push_back(+560);
			Raw.push_back(-45000);

			/*
			Raw.push_back(-1650);
			Raw.push_back(-1650);
			Raw.push_back(+8900);
			Raw.push_back(-2250);
			Raw.push_back(+560);
			*/
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
