/*
*    IRlib.cpp
*    Class resolve different IR signals
*
*/

#include "IRLib.h"

IRLib::IRLib(vector<int32_t> Raw) {
	RawData = Raw;

	LoadDataFromRaw();
}

IRLib::IRLib(string ProntoHex) {
	Converter::FindAndRemove(ProntoHex, " ");
	FillFromProntoHex(ProntoHex);
	LoadDataFromRaw();
}

void IRLib::LoadDataFromRaw() {
	Protocol = GetProtocol();

	switch (Protocol) {
		case NEC: Uint32Data = NECData(); break;
		default:
			Uint32Data = 0;
	}
}

void IRLib::FillRawData() {
	switch (Protocol) {
		case NEC: RawData = NECConstruct(); break;
		default:
			break;
	}
}

string IRLib::GetProntoHex() {
	return ProntoHexConstruct();
}

void IRLib::SetFrequency(uint16_t Freq) {
	if (Freq < 30000 || Freq > 60000) {
		Frequency = 0;
		return;
	}

	Frequency = round(Freq/1000) * 1000;

	if 	(Frequency > 35000 && Frequency <= 37000)
		Frequency = 36000;
	else if (Frequency > 37000 && Frequency <= 39000)
		Frequency = 38000;
	else if (Frequency > 39000 && Frequency <= 42000)
		Frequency = 40000;
	else if (Frequency > 52000 && Frequency <= 60000)
		Frequency = 56000;
}

IRLib::ProtocolEnum IRLib::GetProtocol() {
	if (IsNEC()) return NEC;

	return RAW;
}

bool IRLib::IsNEC() {
	if (RawData.size() >= 66) {
		if (RawData.at(0) 	> 8900 	&& RawData.at(0) 	< 9100 &&
			RawData.at(1) 	< -4350 && RawData.at(1) 	> -4650 &&
			RawData.at(66) 	> 500 	&& RawData.at(66)	< 700)
			return true;
	}

	if (RawData.size() == 16) {
		if (RawData.at(0) > 8900 && RawData.at(0) < 9100 &&
			RawData.at(1) < -2000 && RawData.at(1) > -2400 &&
			RawData.at(2) > 500 && RawData.at(2) < 700)
		return true;
	}

	return false;
}

uint32_t IRLib::NECData() {
	uint16_t 	Address = 0x0;
	uint8_t		Command = 0x0;

	vector<int32_t> Data = RawData;

    Data.erase(Data.begin());
    Data.erase(Data.begin());

    if (Data.size() == 16) { // repeat signal
    		return 0x00FFFFFF;
    }

    for (uint8_t BlockId = 0; BlockId < 4; BlockId++) {
    		bitset<8> Block;

		for (int i=0; i<8; i++) {
			if (Data[0] > 500 && Data[0] < 720) {
				if (Data[1] > -700)
					Block[i] = 0;
				if (Data[1] < -1200)
					Block[i] = 1;
			}

			Data.erase(Data.begin()); Data.erase(Data.begin());
		}

		uint8_t BlockInt = (uint8_t)Block.to_ulong();

		switch (BlockId) {
			case 0: Address = BlockInt; break;
			case 1:
				if ((uint8_t)Address != (uint8_t)~BlockInt) {
					uint16_t tmpAddress = BlockInt;
					tmpAddress = tmpAddress << 8;
					Address += tmpAddress;
				}
				break;
			case 2: Command = BlockInt; break;
			case 3:
				if (Command != (uint8_t)~BlockInt) Command = 0;
				break;
		}
    }

    return (Address << 8) + Command;
};

vector<int32_t> IRLib::NECConstruct() {
	uint16_t	 NECAddress = (uint16_t)((Uint32Data >> 8));
	uint8_t 	 NECCommand = (uint8_t) ((Uint32Data << 24) >> 24);

	vector<int32_t> Data = vector<int32_t>();

	/* raw NEC header */
	Data.push_back(+9000);
	Data.push_back(-4500);

	if (NECAddress <= 0xFF)
		NECAddress += (((uint8_t)~NECAddress) << 8);

	bitset<16> AddressItem(NECAddress);
	for (int i=0;i<16;i++) {
		Data.push_back(+560);
		Data.push_back((AddressItem[i] == true) ? -1650 : -560);
	}

	bitset<8> CommandItem(NECCommand);
	for (int i=0;i<8;i++) {
		Data.push_back(+560);
		Data.push_back((CommandItem[i] == true) ? -1650 : -560);
	}

	for (int i=0;i<8;i++) {
		Data.push_back(+560);
		Data.push_back((CommandItem[i] == false) ? -1650 : -560);
	}

	Data.push_back(+560);
	Data.push_back(-1650);
	Data.push_back(-1650);
	Data.push_back(+8900);
	Data.push_back(-2250);
	Data.push_back(+560);

	return Data;
}

bool IRLib::IsProntoHex() {
	return true;
}

void IRLib::FillFromProntoHex(string SrcString) {
	//uint8_t	Learned = Converter::UintFromHexString<uint8_t>(SrcString.substr(0,4));
	Frequency = (uint16_t)(1000000/((Converter::UintFromHexString<uint16_t>(SrcString.substr(4,4)))* 0.241246));

	uint8_t OneTimeBurstLength 	= Converter::UintFromHexString<uint8_t>(SrcString.substr(8,4));
	uint8_t BurstPairLength 		= Converter::UintFromHexString<uint8_t>(SrcString.substr(12,4));
	uint8_t USec					= (uint8_t)(((1.0 / Frequency) * 1000000) + 0.5);

	SrcString = SrcString.substr(16);

	ProntoOneTimeBurst 	= OneTimeBurstLength;
	ProntoRepeatBurst	= BurstPairLength;

	RawData.clear();
	while (SrcString.size() >= 8) {
		RawData.push_back(+USec * Converter::UintFromHexString<uint16_t>(SrcString.substr(0,4)));
		RawData.push_back(-USec * Converter::UintFromHexString<uint16_t>(SrcString.substr(4,4)));
		SrcString = SrcString.substr(8);
	}
}

string IRLib::ProntoHexConstruct() {
	uint8_t	USec	 = (uint8_t)(((1.0 / Frequency) * 1000000) + 0.5);
	string	Result = "0000";

	Result += " " + Converter::ToHexString((uint8_t)floor(1000000/(Frequency*0.241246)), 4);

	if (	ProntoOneTimeBurst == 0 && ProntoRepeatBurst == 0) {
		Result += " " + Converter::ToHexString(ceil(RawData.size() / 2), 4);
		Result += " " + Converter::ToHexString(0, 4);
	}
	else {
		Result += " " + Converter::ToHexString(ProntoOneTimeBurst, 4);
		Result += " " + Converter::ToHexString(ProntoRepeatBurst, 4);
	}


	for (auto &Bit : RawData)
		Result += " " + Converter::ToHexString(round(abs(Bit) / USec) , 4);

	return Result;
}
