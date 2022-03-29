/*
 *    RC6.cpp
 *    Class for RC6 protocol
 *
 */

class RC6 : public IRProto {
	private:
		const uint16_t NormalBitHalfLen 	= 444;
		const uint16_t TrailerBitHalfLen 	= 889;

		bool isOneBit(int32_t FirstPart, int32_t SecondPart, uint16_t BitHalfLen) {
			return ((FirstPart == BitHalfLen) && (SecondPart == -BitHalfLen));
		}

		void AddBit(vector<int32_t> &Data, bool IsOne, uint16_t BitHalfLen) {
			if (IsOne)
			{
				if (Data.size() > 0 && Data.back() > 0)
					Data.back() = Data.back() + BitHalfLen;
				else
					Data.push_back(BitHalfLen);

				Data.push_back(-BitHalfLen);
			}
			else
			{
				if (Data.size() > 0 && Data.back() < 0)
					Data.back() = Data.back() - BitHalfLen;
				else
					Data.push_back(-BitHalfLen);

				Data.push_back(+BitHalfLen);
			}
		}

	public:
		RC6() {
			ID 					= 0x08;
			Name 				= "RC6";
			DefinedFrequency	= 36000;
		};

		bool IsProtocol(vector<int32_t> &RawData, uint16_t Start, uint16_t Length) override {
			if (RawData.size() < 10)
				return false;

			if (!TestValue(RawData[0], 2666) || !TestValue(RawData[1], -TrailerBitHalfLen))
				return false;

			vector<int32_t> NormalizedData = NormalizeData(RawData);

			if (NormalizedData.size() != 46)
				return false;

			if (!(abs(NormalizedData[10]) == NormalBitHalfLen) || !(abs(NormalizedData[11]) == NormalBitHalfLen))
				return false;

			if (!(abs(NormalizedData[12]) == NormalBitHalfLen) || !(abs(NormalizedData[13]) == NormalBitHalfLen))
				return false;

			return true;
		}

		vector<int32_t> NormalizeData(vector<int32_t> &RawData) {
			vector<int32_t> Data = vector<int32_t>();

			if (RawData.size() < 2)
				return Data;

			Data.push_back(RawData[0]);
			Data.push_back(RawData[1]);

			for (int i = 2; i < RawData.size(); i++)
			{
				int Sign = (RawData[i] > 0) ? 1 : -1;

				int32_t Value = Sign * NormalBitHalfLen;

				if (TestValue(abs(RawData[i]), NormalBitHalfLen, 0.3)) {
					Data.push_back(Value);
				}
				else if (TestValue(abs(RawData[i]), NormalBitHalfLen * 2, 0.3)) {
					Data.push_back(Value);
					Data.push_back(Value);
				}
				else if (TestValue(abs(RawData[i]), NormalBitHalfLen * 3, 0.3)) {
					Data.push_back(Value);
					Data.push_back(Value);
					Data.push_back(Value);
				}
			}

			return Data;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {
			uint32_t Uint32Data = 0;

			vector<int32_t> Data = NormalizeData(RawData);

			Data.erase(Data.begin(), Data.begin() + 4);

			// Get 3-bits mode code
		    bitset<3> CodeModeBits;
			for (int i=0; i < 3; i++) {
				CodeModeBits[2-i] = isOneBit(Data[i*2], Data[i*2+1], NormalBitHalfLen);
				Data.erase(Data.begin(), Data.begin() + 2);
			}
			Uint32Data += (uint32_t)CodeModeBits.to_ulong();
			Uint32Data = Uint32Data << 8;

			// Get TR part
			if (Data[0] == NormalBitHalfLen && Data[1] == NormalBitHalfLen)
				Uint32Data += 1;
			else
				Uint32Data += 0;

			Data.erase(Data.begin(), Data.begin() + 4);
			Uint32Data = Uint32Data << 8;

			// Get 8 control bits
		    bitset<8> ControlBits;
			for (int i=0; i < 8; i++)
				ControlBits[7-i] = isOneBit(Data[i*2], Data[i*2+1], NormalBitHalfLen);

			Data.erase(Data.begin(), Data.begin() + 16);

			Uint32Data += (uint32_t)ControlBits.to_ulong();
			Uint32Data = Uint32Data << 8;

			// Get 8 information bits
		    bitset<8> InformationBits;
			for (int i=0; i < 8; i++) {
				InformationBits[7-i] = isOneBit(Data[i*2], Data[i*2+1], NormalBitHalfLen);
			}

			Uint32Data += (uint32_t)InformationBits.to_ulong();

			return make_pair(Uint32Data, 0x0);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			Raw.push_back(2666);
			Raw.push_back(-889);

			AddBit(Raw, true, NormalBitHalfLen);

			// 3 mode code bits in header
			bitset<3> ModeCodeBits((uint8_t)((Data & 0xFF000000) >> 24));
			for (int i=2; i>=0; i--)
				AddBit(Raw, ModeCodeBits[i], NormalBitHalfLen);

			AddBit(Raw, (Data & 0x00FF0000) != 0x0, TrailerBitHalfLen);

			// 8 control bits in header
			bitset<8> ControlBits((uint8_t)((Data & 0x0000FF00) >> 8));
			for (int i=7; i>=0; i--)
				AddBit(Raw, ControlBits[i], NormalBitHalfLen);

			// 8 control bits in header
			bitset<8> InformationBits((uint8_t)((Data & 0x000000FF)));
			for (int i=7; i>=0; i--)
				AddBit(Raw, InformationBits[i], NormalBitHalfLen);

			Raw.push_back(-45000);

			return Raw;
		}

		vector<int32_t> ConstructRawRepeatSignal(uint32_t Data, uint16_t Misc) override {
			return ConstructRaw(Data, Misc);
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) {
			/*
			bitset<32> Bits(Data);
			Bits[17] = !Bits[17];

			ESP_LOGE("Inverted", "Source data %08X, Output data %08X", Data, (uint32_t)Bits.to_ulong());

			return ConstructRaw((uint32_t)Bits.to_ulong(), Misc);
			*/

			return ConstructRaw(Data, Misc);
		}

};
