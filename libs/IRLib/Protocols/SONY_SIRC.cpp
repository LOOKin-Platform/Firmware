/*
 *    SONY_SIRC.cpp
 *    Class for SONY SIRC protocol
 *
 */

class SONY_SIRC : public IRProto {
	public:
		SONY_SIRC() {
			ID 					= 0x02;
			Name 				= "SIRC";
			DefinedFrequency	= 40000;
		};

		bool IsProtocol(vector<int32_t> RawData) override {		
			if (RawData.size() >= 25) {
				if (RawData.at(0) 	> 2000 	&& RawData.at(0) 	< 3000 &&
					RawData.at(1) 	< -500 && RawData.at(1) 	> -600)
					return true;
			}

			return false;
		}

		uint32_t GetData(vector<int32_t> RawData) override {
			uint8_t 	Command 	= 0x00;
			uint8_t		Device 		= 0x00;
			uint8_t		Extended 	= 0x00;

		    if (RawData.size() < 13*2)
		    	return 0x00;

		    uint32_t	Result = 0x0;

			vector<int32_t> Data = RawData;

		    Data.erase(Data.begin());
		    Data.erase(Data.begin());

    		bitset<7> CommandBlock;

			for (int i = 0; i < 7; i++)
				if (Data[i*2 + 1] > -720 && Data[i*2 + 1] < -500)
					CommandBlock[i] = (Data[i*2] > 1000) ? 1 : 0;

			Command = (uint8_t)CommandBlock.to_ulong();
			Data.erase(Data.begin(), Data.begin() + 14);

			if (Data.size() <= 7*2 || Data.size() > 17*2)
			{
	    		bitset<5> DeviceBlock;

				for (int i=0; i < 5; i++)
					if (Data[i*2 + 1] > -720 && Data[i*2 + 1] < -500)
						DeviceBlock[i] = (Data[i*2] > 1000) ? 1 : 0;

				Device = (uint8_t)DeviceBlock.to_ulong();
				Data.erase(Data.begin(), Data.begin() + 10);

				if (Data.size() >= 8*2)
				{
		    		bitset<8> ExtendedBlock;

					for (int i=0; i < 8; i++)
						if (Data[i*2 + 1] > -720 && Data[i*2 + 1] < -500)
							ExtendedBlock[i] = (Data[i*2] > 1000) ? 1 : 0;

					Extended = (uint8_t)ExtendedBlock.to_ulong();

					Data.erase(Data.begin(), Data.begin() + 16);
				}
			}
			else if (Data.size() >= 15*2)
			{
				ESP_LOGI("!","4");

	    		bitset<5> DeviceBlock;

				for (int i=0; i < 8; i++)
					if (Data[i*2 + 1] > -720 && Data[i*2 + 1] < -500)
						DeviceBlock[i] = (Data[i*2] > 1000) ? 1 : 0;

				Device = (uint8_t)DeviceBlock.to_ulong();
			}

			Result += Command;
			Result  = Result << 8;

			Result += Device;

			if (Extended > 0) {
				Result = Result << 8;
				Result += Extended;
			}

			return Result;
		}

		vector<int32_t> ConstructRaw(uint32_t Data) override {
			uint8_t 	Command 	= 0x00;
			uint8_t		Device 		= 0x00;
			uint8_t		Extended 	= 0x00;

			if (Data <= 0xFFFF) {
				Command = (uint8_t)((Data & 0x0000FF00) >> 8);
				Device 	= (uint8_t)(Data & 0x000000FF);
			}
			else
			{
				Command = (uint8_t)((Data & 0x00FF0000) >> 16);
				Device 	= (uint8_t)((Data & 0x0000FF00) >> 8);
				Extended= (uint8_t)(Data & 0x000000FF);
			}

			vector<int32_t> Raw = vector<int32_t>();

			/* raw NEC header */
			Raw.push_back(+2500);
			Raw.push_back(-500);

			if (Device == 0x0 && Command == 0x0)
				return Raw;

			bitset<7> CommandItem(Command);
			for (int i = 0; i < 7; i++) {
				Raw.push_back((CommandItem[i] == true) ? 1200 : 600);
				Raw.push_back(-600);
			}

			if (Extended > 0x0 || Device < 32)
			{
				bitset<5> DeviceItem(Device);
				for (int i = 0; i < 5; i++) {
					Raw.push_back((DeviceItem[i] == true) ? 1200 : 600);
					Raw.push_back(-600);
				}
			}
			else
			{
				bitset<8> DeviceItem(Device);
				for (int i = 0; i < 8; i++) {
					Raw.push_back((DeviceItem[i] == true) ? 1200 : 600);
					Raw.push_back(-600);
				}
			}

			if (Extended > 0x0 && Device < 32) {
				bitset<8> ExtendedItem(Extended);
				for (int i = 0; i < 8; i++) {
					Raw.push_back((ExtendedItem[i] == true) ? 1200 : 600);
					Raw.push_back(-600);
				}
			}

			Raw.push_back(-46000);

			return Raw;
		}

		vector<int32_t> ConstructRawForSending(uint32_t Data) {
			vector<int32_t> Result = ConstructRaw(Data);
			Result.insert(Result.end(), Result.begin(), Result.end());

			return Result;
		}
};
