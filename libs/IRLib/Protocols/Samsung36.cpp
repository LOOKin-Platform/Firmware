/*
 *    Samsung36.cpp
 *    Class for Samsung36 protocol
 *
 */

#define SAMSUNG36_HDR_MARK		4500
#define SAMSUNG36_HDR_SPACE		4500

#define SAMSUNG36_BIT_MARK		560
#define SAMSUNG36_ONE_SPACE		1680
#define SAMSUNG36_ZERO_SPACE	560

class Samsung36 : public IRProto {
	public:
		Samsung36() {
			ID 					= 0x06;
			Name 				= "Samsung36";
			DefinedFrequency	= 38000;
		};

		bool IsProtocol(vector<int32_t> &RawData, uint16_t Start, uint16_t Length) override {
			if (Length != 78)
				return false;

			if (TestValue(RawData.at(Start + 0), SAMSUNG36_HDR_MARK) &&
				TestValue(RawData.at(Start + 1), -SAMSUNG36_HDR_SPACE) &&
				TestValue(RawData.at(Start + 2), SAMSUNG36_BIT_MARK) &&
				TestValue(RawData.at(Start + 35), -SAMSUNG36_HDR_SPACE))
				return true;

			return false;
		}

		pair<uint32_t, uint16_t> GetData(vector<int32_t> RawData) override {
			vector<int32_t> Raw = RawData;

			uint32_t Data = 0x0;
			uint16_t Misc = 0x0;

			uint32_t Result  = 0x0;

			Raw.erase(Raw.begin());
			Raw.erase(Raw.begin());

		    for (int i = 0; i < 36; i++)
		    {
		    	if (i == 16)
		    	{
		    		Raw.erase(Raw.begin());
		    		Raw.erase(Raw.begin());
		    	}

		    	if (i == 32) {
		    		Data = Result;
		    		Result = 0x0;
		    	}

		    	if (!TestValue(Raw.front(), SAMSUNG36_BIT_MARK))
			    	return make_pair(0x0, 0x0);

		    	Raw.erase(Raw.begin());

				if      (TestValue(Raw.front(), -SAMSUNG36_ONE_SPACE)) 	Result = (Result << 1) | 1 ;
				else if	(TestValue(Raw.front(), -SAMSUNG36_ZERO_SPACE))	Result = (Result << 1) | 0 ;
				else return make_pair(0x0, 0x0);

				Raw.erase(Raw.begin());
		    }

		    Misc = Result;

		    return make_pair(Data, Misc);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			Raw.push_back(SAMSUNG36_HDR_MARK);
			Raw.push_back(-SAMSUNG36_HDR_SPACE);

			bitset<32> DataBits(Data);
			for (int i = 31; i >= 16; i--)
			{
				Raw.push_back(SAMSUNG36_BIT_MARK);
				Raw.push_back((DataBits[i]) ? -SAMSUNG36_ONE_SPACE : -SAMSUNG36_ZERO_SPACE);
			}

			Raw.push_back(SAMSUNG36_BIT_MARK);
			Raw.push_back(-SAMSUNG36_HDR_MARK);

			for (int i = 15; i >= 0; i--)
			{
				Raw.push_back(SAMSUNG36_BIT_MARK);
				Raw.push_back((DataBits[i]) ? -SAMSUNG36_ONE_SPACE : -SAMSUNG36_ZERO_SPACE);
			}

			bitset<16> MiscBits(Misc);
			for (int i = 3; i >=0 ; i--)
			{
				Raw.push_back(SAMSUNG36_BIT_MARK);
				Raw.push_back((MiscBits[i]) ? -SAMSUNG36_ONE_SPACE : -SAMSUNG36_ZERO_SPACE);
			}

			Raw.push_back(+560);
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
