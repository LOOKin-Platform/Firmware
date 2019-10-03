/*
 *    NEC1.cpp
 *    Class for NEC1 protocol
 *
 */

#define AIWA_HDR_MARK		8800
#define AIWA_HDR_SPACE		4500

#define AIWA_BIT_MARK		600
#define AIWA_ONE_SPACE		500
#define AIWA_ZERO_SPACE		1700

class Aiwa : public IRProto {
	public:
		Aiwa() {
			ID 					= 20;
			Name 				= "Aiwa";
			DefinedFrequency	= 38000;
		};

		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() == 88) {
				if (TestValue(RawData.at(0), AIWA_HDR_MARK) &&
					TestValue(RawData.at(1), -AIWA_HDR_SPACE) &&
					TestValue(RawData.at(2), AIWA_BIT_MARK))
					return true;
			}

			return false;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {
			uint32_t	Data = 0x0;
			uint16_t	Misc = 0x0;

			vector<int32_t> Raw = RawData;

			Raw.erase(Raw.begin());
			Raw.erase(Raw.begin());

			bitset<42> BitData;
			for (int j = 0; j < 42; j++)
			{
				if (!IRProto::TestValue(Raw.front(), AIWA_BIT_MARK))
					return make_pair(0x0, 0x0);

			    Raw.erase(Raw.begin());

				if      (IRProto::TestValue(Raw.front(), -AIWA_ONE_SPACE)) 	BitData[j] = 1;
				else if	(IRProto::TestValue(Raw.front(), -AIWA_ZERO_SPACE))	BitData[j] = 0;
				else
					return make_pair(0x0, 0x0);

			   	Raw.erase(Raw.begin());
			}

			uint64_t AiwaData = (uint64_t)(BitData.to_ullong());

			Data = (uint32_t)(AiwaData >> 10);
			Misc = (uint16_t)((AiwaData << 54) >> 54);

		    return make_pair(Data, Misc);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Raw = vector<int32_t>();

			/* raw NEC header */
			Raw.push_back(AIWA_HDR_MARK);
			Raw.push_back(-AIWA_HDR_SPACE);

			bitset<32> DataBitset(Data);
			for (int i = 0;i < 32;i++) {
				Raw.push_back(+AIWA_BIT_MARK);
				Raw.push_back((DataBitset[i] == true) ? -AIWA_ONE_SPACE : -AIWA_ZERO_SPACE);
			}

			bitset<10> MiscBitset(Misc);
			for (int i = 0;i < 32;i++) {
				Raw.push_back(+AIWA_BIT_MARK);
				Raw.push_back((MiscBitset[i] == true) ? -AIWA_ONE_SPACE : -AIWA_ZERO_SPACE);
			}

			Raw.push_back(+AIWA_BIT_MARK);
			Raw.push_back(-25000);

			return Raw;
		}

		vector<int32_t> ConstructRawRepeatSignal(uint32_t Data, uint16_t Misc) override {
			vector<int32_t> Result = vector<int32_t>();

			Result.push_back(+9000);
			Result.push_back(-4500);
			Result.push_back(+600);
			Result.push_back(-25000);

			return Result;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) {
			return ConstructRaw(Data, Misc);
		}

};
