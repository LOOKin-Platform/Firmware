/*
 *    NECx.cpp
 *    Class for NECx protocol
 *
 */


#define NECX_HDR_MARK		4500
#define NECX_HDR_SPACE		4500

#define NECX_BIT_MARK		560
#define NECX_ONE_SPACE		1600
#define NECX_ZERO_SPACE		560

class NECx : public IRProto {
	public:
		NECx() {
			ID 					= 0x04;
			Name 				= "NECx";
			DefinedFrequency	= 38000;
		};

		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() < 66)
				return false;

			if (TestValue(RawData.at(0), NECX_HDR_MARK) &&
				TestValue(RawData.at(1), -NECX_HDR_SPACE) &&
				TestValue(RawData.at(2), NECX_BIT_MARK) &&
				RawData.size() == 68)
				return true;

			return false;
		}

		pair<uint32_t, uint16_t> GetData(vector<int32_t> RawData) override {
			vector<int32_t> Data = RawData;
			uint32_t Result = 0x0;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

		    if (Data.size() < 10)
		    	return make_pair(0xFFFFFFFF,0x0); // repeat signal

		    if (Data.size() < 64)
		    	return make_pair(0x0, 0x0);

		    for (int i=0; i<32; i++)
		    {
		    	if (!TestValue(Data.front(), 560))
			    	return make_pair(0x0, 0x0);

			    Data.erase(Data.begin());

				if      (TestValue(Data.front(), -1600)) 	Result = (Result << 1) | 1 ;
				else if	(TestValue(Data.front(), -560))		Result = (Result << 1) | 0 ;
				else return make_pair(0x0, 0x0);

			    Data.erase(Data.begin());
		    }

		    return make_pair(Result, 0x0);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			Raw.push_back(+4500);
			Raw.push_back(-4500);

			bitset<32> DataBits(Data);

			for (int i = 31; i >= 0; i--)
			{
				Raw.push_back(+560);
				Raw.push_back((DataBits[i]) ? -1600 : -560);
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
