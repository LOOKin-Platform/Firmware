/*
 *    Samsung.cpp
 *    Class for Samsung protocol
 *
 */

class Samsung : public IRProto {
	public:
		Samsung() {
			ID 					= 0x04;
			Name 				= "Samsung";
			DefinedFrequency	= 38500;
		};

		bool IsProtocol(vector<int32_t> RawData) override {
			if (RawData.size() < 66)
				return false;

			if (TestValue(RawData.at(0), 4500) && TestValue(RawData.at(1), -4500))
				return true;

			return false;
		}

		uint32_t GetData(vector<int32_t> RawData) override {
			vector<int32_t> Data = RawData;
			uint32_t Result = 0x0;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

		    if (Data.size() < 10)
		    	return 0xFFFFFFFF; // repeat signal

		    if (Data.size() < 64)
		    	return 0x00;

		    for (int i=0; i<32; i++)
		    {
		    	if (!TestValue(Data.front(), 560))
		    		return 0x00;

			    Data.erase(Data.begin());

				if      (TestValue(Data.front(), -1600)) 	Result = (Result << 1) | 1 ;
				else if	(TestValue(Data.front(), -560))		Result = (Result << 1) | 0 ;
				else return 0x00;

			    Data.erase(Data.begin());
		    }

		    return Result;
		}

		vector<int32_t> ConstructRaw(uint32_t Data) override {
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

		vector<int32_t> ConstructRawRepeatSignal(uint32_t Data) override {
			return ConstructRaw(Data);
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data) {
			return ConstructRaw(Data);
		}

};
