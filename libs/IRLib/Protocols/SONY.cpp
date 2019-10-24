/*
 *    SONY_SIRC.cpp
 *    Class for SONY SIRC protocol
 *
 */

#define SONY_BIT_MARK		600
#define SONY_ONE_SPACE		1200
#define SONY_ZERO_SPACE		600

#define SONY_HDR_MARK		2500
#define SONY_HDR_SPACE		600

class SONY : public IRProto {
	public:
		SONY() {
			ID 					= 0x03;
			Name 				= "SONY";
			DefinedFrequency	= 40000;
		};

		bool IsProtocol(vector<int32_t> &RawData) override {
			if (RawData.size() == 26 || RawData.size() == 32 || RawData.size() == 42)
				return (TestValue(RawData.at(0), SONY_HDR_MARK) &&  TestValue(RawData.at(1), -SONY_HDR_SPACE));

			return false;
		}

		pair<uint32_t,uint16_t> GetData(vector<int32_t> RawData) override {
			uint8_t 	Command 	= 0x00;
			uint8_t		Device 		= 0x00;
			uint8_t		Extended 	= 0x00;
			uint8_t		Version		= 0x00;

		    uint32_t	Result 		= 0x0;

			switch (RawData.size()) {
				case 26: Version = 12; break;
				case 32: Version = 15; break;
				case 42: Version = 20; break;
			}

		    if (Version == 0)
		    	return make_pair(0x0, 0x0);

			vector<int32_t> Data = RawData;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

    		bitset<7> CommandBlock;

			for (int i = 0; i < 7; i++)
				if (TestValue(Data[i*2 + 1], -SONY_BIT_MARK))
					CommandBlock[i] = (TestValue(Data[i*2],SONY_ONE_SPACE)) ? 1 : 0;

			Command = (uint8_t)CommandBlock.to_ulong();
			Data.erase(Data.begin(), Data.begin() + 7*2);

			const uint8_t DataFieldSize = (Version == 15) ? 8 : 5;
	    	bitset<8> DeviceBlock;

			for (int i = 0; i < DataFieldSize; i++)
				if (TestValue(Data[i*2 + 1], -SONY_BIT_MARK))
					DeviceBlock[i] = (TestValue(Data[i*2], SONY_ONE_SPACE)) ? 1 : 0;

			Device = (uint8_t)DeviceBlock.to_ulong();
			Data.erase(Data.begin(), Data.begin() + DataFieldSize*2);

			if (Version == 20)
			{
	    		bitset<8> ExtendedBlock;

				for (int i=0; i < 8; i++)
					if (TestValue(Data[i*2 + 1], -SONY_BIT_MARK))
						ExtendedBlock[i] = (TestValue(Data[i*2], SONY_ONE_SPACE)) ? 1 : 0;

				Extended = (uint8_t)ExtendedBlock.to_ulong();
				Data.erase(Data.begin(), Data.begin() + 8*2);
			}

			Result += Version;
			Result  = Result << 8;

			Result += Command;
			Result 	= Result << 8;

			Result += Device;
			Result 	= Result << 8;

			Result += Extended;

			return make_pair(Result, 0x0);
		}

		vector<int32_t> ConstructRaw(uint32_t Data, uint16_t Misc) override {
			uint8_t		Version		= 0x0;
			uint8_t 	Command 	= 0x0;
			uint8_t		Device 		= 0x0;
			uint8_t		Extended 	= 0x0;

			Version = (uint8_t)((Data & 0xFF000000) >> 24);
			Command = (uint8_t)((Data & 0x00FF0000) >> 16);
			Device 	= (uint8_t)((Data & 0x0000FF00) >> 8);
			Extended= (uint8_t)((Data & 0x000000FF));

			vector<int32_t> Raw = vector<int32_t>();

			if ((Version == 0x0) || (Device == 0x0 && Command == 0x0))
				return Raw;

			if (Version != 12 && Version != 15 && Version != 20)
				return Raw;

			Raw.push_back(SONY_HDR_MARK);
			Raw.push_back(-SONY_HDR_SPACE);

			bitset<7> CommandItem(Command);
			for (int i = 0; i < 7; i++) {
				Raw.push_back((CommandItem[i] == true) ? SONY_ONE_SPACE : SONY_ZERO_SPACE);
				Raw.push_back(-SONY_BIT_MARK);
			}

			uint8_t DataFieldSize = (Version == 15) ? 8 : 5;
	    	bitset<8> DeviceItem(Device);
	    	for (int i = 0; i < DataFieldSize; i++) {
				Raw.push_back((DeviceItem[i] == true) ? SONY_ONE_SPACE : SONY_ZERO_SPACE);
				Raw.push_back(-SONY_BIT_MARK);
			}

	    	if (Version == 20) {
		    	bitset<8> ExtendedItem(Extended);

		    	for (int i = 0; i < 8; i++) {
					Raw.push_back((ExtendedItem[i] == true) ? SONY_ONE_SPACE : SONY_ZERO_SPACE);
					Raw.push_back(-SONY_BIT_MARK);
				}
	    	}

	    	Raw.pop_back();
			Raw.push_back(-46000);

			return Raw;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data, uint16_t Misc) {
			vector<int32_t> Result = ConstructRaw(Data, Misc);

			Result.pop_back();
			Result.push_back(-30000);

			Result.insert(Result.end(), Result.begin(), Result.end());

			Result.pop_back();
			Result.push_back(-46000);

			return Result;
		}

		vector<int32_t> ConstructRawRepeatSignal(uint32_t Data, uint16_t Misc) override {
			return ConstructRaw(Data, Misc);
		}
};
