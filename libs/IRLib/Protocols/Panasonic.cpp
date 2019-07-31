/*
 *    Panasonic.cpp
 *    Class for Panasonic protocol
 *
 */

#define PANASONIC_BIT_MARK		460
#define PANASONIC_ONE_SPACE		1200
#define PANASONIC_ZERO_SPACE	460

#define PANASONIC_HDR_MARK		3502
#define PANASONIC_HDR_SPACE		1750

class Panasonic : public IRProto {
	public:
		Panasonic() {
			ID 					= 0x05;
			Name 				= "Panasonic";
			DefinedFrequency	= 37000;
		};

		bool IsProtocol(vector<int32_t> RawData) override {
			if (RawData.size() < 49)
				return false;

			if (TestValue(RawData.at(0), PANASONIC_HDR_MARK) && TestValue(RawData.at(1), -PANASONIC_HDR_SPACE) && TestValue(RawData.at(2), PANASONIC_BIT_MARK))
				return true;

			return false;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {
			vector<int32_t> Data = RawData;
			uint32_t Result = 0x0;
			uint16_t Misc 	= 0x0;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

		    if (Data.size() < 97)
		    	return make_pair(0x0,0x0);

		    for (int i=0; i<32; i++)
		    {
		    	if (!TestValue(Data.front(), PANASONIC_BIT_MARK))
			    	return make_pair(0x0,0x0);

			    Data.erase(Data.begin());

				if      (TestValue(Data.front(), -PANASONIC_ONE_SPACE)) 	Misc = (Result << 1) | 1 ;
				else if	(TestValue(Data.front(), -PANASONIC_ZERO_SPACE))	Misc = (Result << 1) | 0 ;
				else return make_pair(0x0,0x0);

			    Data.erase(Data.begin());
		    }

		    for (int i=0; i<64; i++)
		    {
		    	if (!TestValue(Data.front(), PANASONIC_BIT_MARK))
			    	return make_pair(0x0,0x0);

			    Data.erase(Data.begin());

				if      (TestValue(Data.front(), -PANASONIC_ONE_SPACE)) 	Result = (Result << 1) | 1 ;
				else if	(TestValue(Data.front(), -PANASONIC_ZERO_SPACE))	Result = (Result << 1) | 0 ;
				else return make_pair(0x0,0x0);

			    Data.erase(Data.begin());
		    }

		    return make_pair(Result, Misc);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			Raw.push_back(+PANASONIC_HDR_MARK);
			Raw.push_back(-PANASONIC_HDR_SPACE);

			bitset<32> DataBits(Data);
			bitset<16> MiscBits(Misc);

			for (int i = 15; i >= 0; i--)
			{
				Raw.push_back(+PANASONIC_BIT_MARK);
				Raw.push_back((MiscBits[i]) ? -PANASONIC_ONE_SPACE : -PANASONIC_ZERO_SPACE);
			}

			for (int i = 31; i >= 0; i--)
			{
				Raw.push_back(+PANASONIC_BIT_MARK);
				Raw.push_back((DataBits[i]) ? -PANASONIC_ONE_SPACE : -PANASONIC_ZERO_SPACE);
			}

			Raw.push_back(+PANASONIC_BIT_MARK);
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
