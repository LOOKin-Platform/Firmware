/*
 *    RC5.cpp
 *    Class for RC5 protocol
 *
 */

class RC5 : public IRProto {
	private:
		const int32_t HalfBitLen = 889;

		bool isOneBit(int32_t FirstPart, int32_t SecondPart) {
			return ((TestValue(FirstPart, -HalfBitLen)) && (TestValue(SecondPart, HalfBitLen)));
		}

		void AddBit(vector<int32_t> &Data, bool IsOne) {
			if (IsOne)
			{
				if (Data.size() > 0 && Data.back() < 0)
					Data.back() = -HalfBitLen * 2;
				else
					Data.push_back(-HalfBitLen);

				Data.push_back(+HalfBitLen);
			}
			else
			{
				if (Data.size() > 0 && Data.back() > 0)
					Data.back() = HalfBitLen * 2;
				else
					Data.push_back(HalfBitLen);

				Data.push_back(-HalfBitLen);
			}
		}

	public:
		RC5() {
			ID 					= 0x07;
			Name 				= "RC5";
			DefinedFrequency	= 36000;
		};

		bool IsProtocol(vector<int32_t> &RawData, uint16_t Start, uint16_t Length) override {

			int Counter = 1;

			for (int i= Start; i < Length; i++)
			{
				if (RawData.at(i) == -Settings.SensorsConfig.IR.SignalEndingLen)
					break;

				if (TestValue((int32_t)abs(RawData.at(i)), HalfBitLen))
					Counter += 1;
				else if (TestValue((int32_t)abs(RawData.at(i)), HalfBitLen*2))
					Counter += 2;
				else
					return false;
			}

			if (Counter == 27 || Counter == 28)
				return true;

			return false;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {

			vector<int32_t> Data = vector<int32_t>();

			for (int32_t& DataItem : RawData) {
				int Sign = (DataItem > 0) ? 1 : -1;

				if (TestValue(abs(DataItem), HalfBitLen))
					Data.push_back(Sign * HalfBitLen);
				if (TestValue(abs(DataItem), HalfBitLen * 2)) {
					Data.push_back(Sign * HalfBitLen);
					Data.push_back(Sign * HalfBitLen);
				}
			}

			// check first bit. If not 1 - not RC5 protocol
			if (!isOneBit(-HalfBitLen, Data.front()))
				return make_pair(0xFFFFFFFF,0x0);

			// skip first bit always 1
			Data.erase(Data.begin());

			// if last bit is missed - add it
			if (Data.size() == 25)
				Data.push_back(-HalfBitLen);

			if (Data.size() != 26)
				return make_pair(0xFFFFFFFF,0x0);

		    bitset<13> Bits;

			// second bit is always 1 for RC5 and may be 1 or 2 for RC5x
			for (int i=0; i < 13; i++)
				Bits[12-i] = isOneBit(Data[i*2], Data[i*2+1]);

			uint32_t Uint32Data = 0x0;

			Uint32Data = ((Bits[12]) ? 1 : 0);
			Uint32Data = ((Uint32Data << 8) + ((Bits[11]) ? 1 : 0));

			Bits[12] = false;
			Bits[11] = false;

			uint16_t AddressAndCommand = (uint16_t)Bits.to_ulong();

			uint8_t Address = (uint8_t)(AddressAndCommand >> 6);
			uint8_t Command = (uint8_t)(AddressAndCommand & 0b0000000000111111);

			Uint32Data = (Uint32Data << 8) + Address;
			Uint32Data = (Uint32Data << 8) + Command;

			return make_pair(Uint32Data, 0x0);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			Raw.push_back(HalfBitLen);

			// 2-3 bit
			AddBit(Raw, (Data & 0xFF000000) != 0x0);
			AddBit(Raw, (Data & 0x00FF0000) != 0x0);

			uint8_t Address = (uint8_t)((Data & 0x0000FF00) >> 8);
			bitset<5> AddressSignal(Address);
			for (int i=4; i>=0; i--)
				AddBit(Raw, AddressSignal[i]);

			uint8_t Command = (uint8_t)(Data & 0x000000FF);
			bitset<6> CommandSignal(Command);
			for (int i=5; i>=0; i--)
				AddBit(Raw, CommandSignal[i]);

			Raw.push_back(-45000);

			return Raw;
		}

		vector<int32_t> ConstructRawRepeatSignal(uint32_t Data, uint16_t Misc) override {
			return ConstructRaw(Data, Misc);
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) {
			return ConstructRaw(Data, Misc);
		}

};
