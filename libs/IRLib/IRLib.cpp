/*
*    IRlib.cpp
*    Class resolve different IR signals
*
*/

#include "IRLib.h"

static char tag[] = "IRLib";

vector<IRProto *> IRLib::Protocols = vector<IRProto *>();

IRLib::IRLib(vector<string> Raw) {

	for (string Item : Raw)
		RawData.push_back(Converter::ToInt32(Item));

	FillProtocols();
	LoadDataFromRaw();
}

IRLib::IRLib(vector<int32_t> Raw) {
	RawData = Raw;

	FillProtocols();
	LoadDataFromRaw();
}

IRLib::IRLib(const char *ProntoHex) {
	//	Converter::FindAndRemove(ProntoHex, " ");

	FillProtocols();

	if (FillFromProntoHex(ProntoHex))
		LoadDataFromRaw();
}

IRLib::IRLib(string &ProntoHex) {
	Converter::FindAndRemove(ProntoHex, " ");

	FillProtocols();

	if (FillFromProntoHex(ProntoHex))
		LoadDataFromRaw();
}

void IRLib::ExternFillPostOperations() {
	FillProtocols();
	LoadDataFromRaw();
}

void IRLib::LoadFromRawString(string &RawInString) {
	RawData.clear();

	while (RawInString.size() > 0)
	{
		size_t Pos = RawInString.find(" ");
		string Item = (Pos != string::npos) ? RawInString.substr(0,Pos) : RawInString;

		RawData.push_back(Converter::ToInt32(Item));

		if (Pos == string::npos || RawInString.size() < Pos)
			RawInString = "";
		else
			RawInString.erase(0, Pos+1);
	}

	FillProtocols();
	LoadDataFromRaw();
}


void IRLib::FillProtocols() {
	if (IRLib::Protocols.size() == 0)
		Protocols = {
			new NEC1(),
			new SONY(),
			new NECx(),
			new Panasonic(),
			new Samsung36(),
			new RC5(),
//			new RC6()
//			new MCE()
//			new Daikin(),
//			new MitsubishiAC(),
//			new Aiwa(),
//			new Gree(),
//			new HaierAC()
		};
}

void IRLib::LoadDataFromRaw() {

	if (RawData.size() > 0) {
		vector <pair<uint32_t,uint16_t>> Pauses = vector<pair<uint32_t,uint16_t>>();

		for (int i=0; i < RawData.size() - 1; i++)
			if (RawData[i] < -5000)
				Pauses.push_back(make_pair(abs(RawData[i]), i));

		if (Pauses.size() > 0) { // long pauses exist
			// first: check if this signal is exact repeated one
			uint32_t FirstItemLength = Pauses[0].second + 1;

			bool IsAllIdentical = true;
			for (int i=0; i < Pauses.size(); i++) {
				uint32_t PartLength = (i != Pauses.size()-1) ? Pauses[i+1].second - Pauses[i].second : RawData.size() - Pauses[i].second - 1;

				if (!CompareIsIdentical(RawData, RawData, 0, Pauses[i].second+1, FirstItemLength - 1, PartLength - 1)) {
					IsAllIdentical = false;
					break;
				}
			}

			if (IsAllIdentical) {
				RawData = std::vector<int32_t>(RawData.begin(), RawData.begin() + FirstItemLength);
				IsRepeated = true;
				RepeatCount = Pauses.size();
				RepeatPause = Pauses[0].first;
			}
			else if (Pauses.size() == 1) {
				// second: check for main signal and repeat command

				IRProto *FirstPartProtocol = GetProtocol(0, FirstItemLength);

				if (FirstPartProtocol != nullptr) {
					vector<int32_t> RepeatSignal = FirstPartProtocol->ConstructRawRepeatSignal(0, 0);
					if (CompareIsIdentical(RawData, RepeatSignal, FirstItemLength, 0, RawData.size() - Pauses[0].second - 1, 0)) {
						RawData = std::vector<int32_t>(RawData.begin(), RawData.begin() + FirstItemLength);
					}
				}
			}
		}

		if (RawData.size() > 0 && RawData[RawData.size() - 1] != -Settings.SensorsConfig.IR.SignalEndingLen)
			RawData[RawData.size() - 1] = -Settings.SensorsConfig.IR.SignalEndingLen;
	}


	IRProto *Protocol = GetProtocol();

	this->Protocol 		= 0xFF;
	this->Uint32Data 	= 0x0;
	this->MiscData		= 0x0;

	if (Protocol != nullptr) {
		this->Protocol 	= Protocol->ID;

		pair<uint32_t, uint16_t> Result = Protocol->GetData(RawData);
		this->Uint32Data= Result.first;
		this->MiscData	= Result.second;
	}

	if (this->Protocol != 0xFF && this->Uint32Data == 0x0 && this->MiscData == 0x0)
		this->Protocol = 0xFF;
}

uint8_t IRLib::GetProtocolExternal(vector<int32_t> &SignalVector){
	FillProtocols();

	if (SignalVector.size() > 0)
	{
		for (auto& Protocol : IRLib::Protocols)
			if (Protocol->IsProtocol(SignalVector, 0, SignalVector.size()))
				return Protocol->ID;
	}

	return 0xFF;

}

void IRLib::FillRawData() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	if (Proto != nullptr)
		this->RawData = Proto->ConstructRaw(this->Uint32Data, this->MiscData);
	else
		this->RawData = vector<int32_t>();
}

string IRLib::GetProntoHex(bool SpaceDelimeter) {
	return ProntoHexConstruct(SpaceDelimeter);
}

string IRLib::GetRawSignal() {
	string SignalOutput = "";

	for (int i=0; i < RawData.size(); i++)
		SignalOutput += Converter::ToString(RawData[i]) + ((i != RawData.size() - 1) ? " " : "");

	return SignalOutput;
}

string IRLib::GetSignalCRC() {
	pair<uint16_t, uint16_t> ZeroPair 	= make_pair(0,0);
	pair<uint16_t, uint16_t> OnePair 	= make_pair(0,0);

	string CRC = "";

	uint8_t CRCItem 	= 0;
	uint8_t BitCounter 	= 0;

	for (int i = 0; i < RawData.size() - 1; i +=2) {
		uint16_t First = (uint16_t)abs(RawData.at(i));
		uint16_t Second= (uint16_t)abs(RawData.at(i+1));

		if (First > 2000 || Second > 2000)
			continue;

		if (ZeroPair.first == 0 && ZeroPair.second == 0)
		{
			ZeroPair.first  = First;
			ZeroPair.second = Second;
		}

		if (CRCCompareFunction(First, ZeroPair.first, 0.4) && CRCCompareFunction(Second, ZeroPair.second, 0.4)) {
			CRCItem = CRCItem << 1;
			CRCItem += 0;
			BitCounter++;
		}
		else
		{
			if (OnePair.first == 0 && OnePair.second == 0)
			{
				OnePair.first  = First;
				OnePair.second = Second;
			}

			if (CRCCompareFunction(First, OnePair.first, 0.4) && CRCCompareFunction(Second, OnePair.second, 0.4)) {
				CRCItem = CRCItem << 1;
				CRCItem += 1;
				BitCounter++;
			}
		}

		if (BitCounter == 4) {
			CRC += Converter::ToHexString(CRCItem, 1);
			CRCItem = 0;
			BitCounter = 0;
		}
	}

	if (BitCounter > 0)
		CRC += Converter::ToHexString(CRCItem, 1);

	return CRC;
}


vector<int32_t>IRLib::GetRawDataForSending() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	vector<int32_t> Result = vector<int32_t>();

	if (Proto != nullptr)
		if (Uint32Data > 0)
			Result = Proto->ConstructRawForSending(this->Uint32Data, this->MiscData);

	if (!IsRepeated)
		return (Result.size() > 0) ? Result : this->RawData;
	else
	{
		vector<int32_t> Part = (Result.size() > 0) ? Result : this->RawData;
		Result = Part;

		for (int i = 0; i < RepeatCount; i++)
		{
			Result.pop_back();
			Result.push_back((RepeatPause > 0) ? -RepeatPause : -25000);
			Result.insert(Result.end(), Part.begin(), Part.end());
		}

		return Result;
	}
}

vector<int32_t> IRLib::GetRawRepeatSignalForSending() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	vector<int32_t> Result = vector<int32_t>();

	if (Proto != nullptr)
		Result = Proto->ConstructRawRepeatSignal(this->Uint32Data, this->MiscData);

	return (Result.size() > 0) ? Result : vector<int32_t>();
}

uint16_t IRLib::GetProtocolFrequency() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	uint16_t Result = 0;

	if (Proto != nullptr)
		Result = Proto->DefinedFrequency;

	return (Result > 0) ? Result : 38000;
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

void IRLib::AppendRawSignal(IRLib &DataToAppend) {
	AppendRawSignal(DataToAppend.RawData);
}

void IRLib::AppendRawSignal(vector<int32_t> &DataToAppend) {
	if (RawData.size() > 0) {
		IRProto *Proto = GetProtocolByID(this->Protocol);

		if (Proto != nullptr)
			RawData.back() = Proto->GetBlocksDelimeter();
	}

	RawData.insert(RawData.end(), DataToAppend.begin(), DataToAppend.end());

	ExternFillPostOperations();

}

int32_t IRLib::RawPopItem() {
	if (RawData.size() == 0)
		return 0;

	int32_t Item = RawData.front();
	RawData.erase(RawData.begin());

	return Item;
}

bool IRLib::CompareIsIdenticalWith(IRLib &Signal) {
	return CompareIsIdentical(*this, Signal);
}

bool IRLib::CompareIsIdenticalWith(vector<int32_t> &SignalInVector) {
	uint16_t SizeDiff		= abs((uint16_t)(RawData.size() - SignalInVector.size()));
	uint16_t MinimalSize	= min(RawData.size(), SignalInVector.size());

	if (SizeDiff >= 5) // too big difference between signals length
		return false;

	for (uint16_t i=0; i< MinimalSize; i++) {
		uint16_t PartDif = abs(RawData[i] - SignalInVector[i]);

		if (PartDif > 0.20 * max(abs(RawData[i]), abs(SignalInVector[i])))
			return false;
	}

	return true;
}


bool IRLib::CompareIsIdentical(IRLib &Signal1, IRLib &Signal2) {
	if (Signal1.Protocol == Signal2.Protocol)
		return CompareIsIdentical(Signal1.RawData, Signal2.RawData);

	return false;
}

bool IRLib::CompareIsIdentical(vector<int32_t> &Signal1, vector<int32_t> &Signal2, uint16_t Signal1Start, uint16_t Signal2Start, uint16_t Signal1Size, uint16_t Signal2Size)
{
	if (Signal1Size == 0) Signal1Size = Signal1.size();
	if (Signal2Size == 0) Signal2Size = Signal2.size();

	uint16_t SizeDiff		= abs((uint16_t)(Signal1Size - Signal2Size));
	uint16_t MinimalSize	= min(Signal1Size, Signal2Size);

	if (SizeDiff >= 5) // too big difference between signals length
		return false;

	for (uint16_t i=0; i< MinimalSize; i++) {
		uint16_t PartDif = abs(Signal1[Signal1Start + i] - Signal2[Signal2Start + i]);

		if (PartDif > 0.20 * max(abs(Signal1[Signal1Start + i]), abs(Signal2[Signal2Start + i])))
			return false;
	}

	return true;
}


IRProto* IRLib::GetProtocol(uint16_t Start, uint16_t Length) {
	if (Length == 0)
		Length = RawData.size();


	if (Length > 0) {
		for (auto& Protocol : IRLib::Protocols)
			if (Protocol->IsProtocol(RawData, Start, Length))
				return Protocol;
	}


	return nullptr;
}

IRProto* IRLib::GetProtocolByID(uint8_t ProtocolID) {
	for (auto& Protocol : IRLib::Protocols)
		if (Protocol->ID == ProtocolID)
			return Protocol;

	return nullptr;
}

bool IRLib::IsProntoHex() {
	return true;
}

bool IRLib::FillFromProntoHex(string &SrcString) {
	if (SrcString.size() < 20) {
		ESP_LOGE(tag, "Too small ProntoHEX data. Skipped");
		return false;
	}

	bool 		IsSkipProntoFlag	= false;

	//uint8_t	Learned = Converter::UintFromHexString<uint8_t>(SrcString.substr(0,4));
	Frequency = (uint16_t)(1000000/((Converter::UintFromHexString<uint16_t>(SrcString.substr(4,4)))* 0.241246));

	if (SrcString.substr(0,4) == "000F")
		IsSkipProntoFlag = true;

	//uint8_t ProntoOneTimeBurst 		= Converter::UintFromHexString<uint8_t>(SrcString.substr(8,4));
	//uint8_t ProntoRepeatBurst 		= Converter::UintFromHexString<uint8_t>(SrcString.substr(12,4));
	uint8_t USec					= (uint8_t)(((1.0 / Frequency) * 1000000) + 0.5);

	SrcString.erase(0,16);

	RawData.clear();
	while (SrcString.size() >= 8) {
		RawData.push_back(+USec * Converter::UintFromHexString<uint16_t>(SrcString.substr(0,4)));
		RawData.push_back(-USec * Converter::UintFromHexString<uint16_t>(SrcString.substr(4,4)));
		SrcString.erase(0,8);
	}

	SrcString.erase();

	return (!IsSkipProntoFlag);
}

bool IRLib::FillFromProntoHex(const char *SrcString) {
	if (strlen(SrcString) < 24) {
		ESP_LOGE(tag, "Too small ProntoHEX data. Skipped");
		return false;
	}

	RawData.clear();

	uint16_t 	SymbolCounter 	= 0;
	string 		Group 			= "";
	bool		Sign			= true;
	uint8_t		USec 			= 0;
	char		SignalChar[1];
	bool 		IsSkipProtoFlag	= false;

	for (int i = 0; i < strlen(SrcString); i++) {
		memcpy(SignalChar, SrcString + i, 1);

		if (string(SignalChar) == string(" "))
			continue;
		else if (string(SignalChar) ==  string("%")) {
			i+=2;
			continue;
		}

		Group += string(SignalChar);

		if (SymbolCounter == 3) {
			if (Group == "000F")
				IsSkipProtoFlag = true;
		}
		else if (SymbolCounter == 7) {
			Frequency 	= (uint16_t)(1000000/((Converter::UintFromHexString<uint16_t>(Group))* 0.241246));
			USec 		= (uint8_t)(((1.0 / Frequency) * 1000000) + 0.5);
		}
		else if (SymbolCounter == 11)
			ProntoOneTimeBurst	= Converter::UintFromHexString<uint8_t>(Group);
		else if (SymbolCounter == 15)
			ProntoRepeatBurst 	= Converter::UintFromHexString<uint8_t>(Group);

		if ((SymbolCounter+1)%4 == 0 ) {
			if (SymbolCounter > 15) {
				uint16_t SignalPart = USec * Converter::UintFromHexString<uint16_t>(Group);
				RawData.push_back((Sign) ? +SignalPart : -SignalPart);
				Sign = !Sign;
			}

			Group = "";
		}
		SymbolCounter++;
	}

	return (!IsSkipProtoFlag);
}

string IRLib::ProntoHexConstruct(bool SpaceDelimeter) {
	uint8_t	USec	 = (uint8_t)(((1.0 / Frequency) * 1000000) + 0.5);
	string	Result = "0000";

	string 	Delimeter = (SpaceDelimeter) ? " " : "";

	Result += Delimeter + Converter::ToHexString((uint8_t)floor(1000000/(Frequency*0.241246)), 4);

	uint16_t RawDataSize = (IsRepeated) ? RawData.size() : ceil(RawData.size() / 2) ;

	if (ProntoOneTimeBurst == 0 && ProntoRepeatBurst == 0) {
		Result += Delimeter + Converter::ToHexString(RawDataSize, 4);
		Result += Delimeter + Converter::ToHexString(0, 4);
	}
	else {
		Result += Delimeter + Converter::ToHexString(ProntoOneTimeBurst, 4);
		Result += Delimeter + Converter::ToHexString(ProntoRepeatBurst, 4);
	}

	for (auto &Bit : RawData)
		Result += Delimeter + Converter::ToHexString(round(abs(Bit) / USec) , 4);

	if (IsRepeated) {
		Result.erase(Result.size()-4, 4);
		Result += Delimeter + Converter::ToHexString(round(RepeatPause / USec) , 4);

		for (auto &Bit : RawData)
			Result += Delimeter + Converter::ToHexString(round(abs(Bit) / USec) , 4);
	}

	return Result;
}

bool IRLib::CRCCompareFunction(uint16_t Item1, uint16_t Item2, float Threshold) {
	if (Item1 > (1-Threshold)*Item2 && Item1 < (1+Threshold)*Item2)
		return true;

	if (Item2 > (1-Threshold)*Item1 && Item2 < (1+Threshold)*Item1)
		return true;

	return false;
}

bool IRLib::TestSignal(vector<int32_t> Input) {
	IRLib Signal(Input);
	vector<int32_t> Output = Signal.GetRawDataForSending();
	return IRLib::CompareIsIdentical(Input, Output);
}

void IRLib::TestAll() {
	// repeat signals test

	bool RepeatTest1 = TestSignal({9024, -4512, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -1692, 564, -1692, 564, -1692, 564, -25000, 9024, -4512, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -564, 564, -1692, 564, -564, 564, -1692, 564, -1692, 564, -1692, 564, -1692, 564, -1692, 564, -1692, 564, -45000});
	ESP_LOGE("RepeatTest1 (repeat)", "%s", RepeatTest1 ? "passed" : "not passed");

	bool RepeatTest2 = TestSignal({3459,-1702,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-1297,432,-73277,3459,-1702,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-1297,432,-73277,3459,-1702,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-1297,432,-73277,3459,-1702,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-432,432,-432,432,-1297,432,-1297,432,-1297,432,-1297,432,-1297,432,-432,432,-1297,432,-45000});
	ESP_LOGE("RepeatTest2 (repeat)", "%s", RepeatTest2 ? "passed" : "not passed");

	bool RepeatTest3 = TestSignal({8630, -4220, 610, -1520, 610, -460, 610, -1530, 610, -460, 610, -460, 580, -1550, 610, -460, 590, -1550, 610, -460, 610, -1530, 590, -480, 610, -1520, 610, -1530, 610, -460, 610, -1530, 610, -460, 610, -460, 610, -460, 610, -1530, 590, -480, 590, -490, 580, -490, 610, -1530, 610, -1530, 610, -1530, 610, -1530, 610, -460, 610, -1530, 610, -1520, 610, -1530, 580, -490, 610, -460, 610, -25000, 8630, -4220, 610, -1520, 610, -460, 610, -1530, 610, -460, 610, -460, 580, -1550, 610, -460, 590, -1550, 610, -460, 610, -1530, 590, -480, 610, -1520, 610, -1530, 610, -460, 610, -1530, 610, -460, 610, -460, 610, -460, 610, -1530, 590, -480, 590, -490, 580, -490, 610, -1530, 610, -1530, 610, -1530, 610, -1530, 610, -460, 610, -1530, 610, -1520, 610, -1530, 580, -490, 610, -460, 610, -45000});
	ESP_LOGE("RepeatTest3 (repeat)", "%s", RepeatTest3 ? "passed" : "not passed");


}
