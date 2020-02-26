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

IRLib::IRLib(string &ProntoHex) {
	Converter::FindAndRemove(ProntoHex, " ");

	FillProtocols();
	FillFromProntoHex(ProntoHex);
	LoadDataFromRaw();
}

void IRLib::ExternFillPostOperations() {
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
			new Daikin(),
			new MitsubishiAC(),
			new Aiwa(),
			new Gree(),
			new HaierAC()
		};
}

void IRLib::LoadDataFromRaw() {
	IRProto *Protocol = GetProtocol();

	this->Protocol 		= 0xFF;
	this->Uint32Data 	= 0x0;
	this->MiscData		= 0x0;

	if (Protocol != nullptr) {
		this->Protocol 	= Protocol->ID;

		pair<uint32_t, uint16_t> Result = Protocol->GetData(RawData);
		this->Uint32Data= Result.first;
		this->MiscData	= Result.second;
		//this->RawData.clear();
	}
}

void IRLib::FillRawData() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	if (Proto != nullptr)
		this->RawData = Proto->ConstructRaw(this->Uint32Data, this->MiscData);
	else
		this->RawData = vector<int32_t>();
}

string IRLib::GetProntoHex() {
	return ProntoHexConstruct();
}

string IRLib::GetRawSignal() {
	string SignalOutput = "";

	for (int i=0; i < RawData.size(); i++)
		SignalOutput += Converter::ToString(RawData[i]) + ((i != RawData.size() - 1) ? " " : "");

	return SignalOutput;
}

vector<int32_t> IRLib::GetRawDataForSending() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	vector<int32_t> Result = vector<int32_t>();

	if (Proto != nullptr)
		if (Uint32Data > 0)
			Result = Proto->ConstructRawForSending(this->Uint32Data, this->MiscData);

	return (Result.size() > 0) ? Result : this->RawData;
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

	if (RawData.size() > 0) {
		IRProto *Proto = GetProtocolByID(this->Protocol);

		if (Proto != nullptr)
			RawData.back() = Proto->GetBlocksDelimeter();
	}

	RawData.insert(RawData.end(), DataToAppend.RawData.begin(), DataToAppend.RawData.end());

	ExternFillPostOperations();
}


int32_t IRLib::RawPopItem() {
	if (RawData.size() == 0)
		return 0;

	int32_t Item = RawData.front();
	RawData.erase(RawData.begin());

	return Item;
}

bool IRLib::CompareIsIdentical(IRLib &Signal1, IRLib &Signal2) {
	if (Signal1.Protocol == Signal2.Protocol) {
		uint16_t SizeDiff		= abs((uint16_t)(Signal1.RawData.size() - Signal2.RawData.size()));
		uint16_t MinimalSize	= min(Signal1.RawData.size(), Signal2.RawData.size());

		if (SizeDiff >= 5) // too big difference between signals length
			return false;

		for (uint16_t i=0; i< MinimalSize; i++) {
			uint16_t PartDif = abs(Signal1.RawData[i] - Signal2.RawData[i]);

			if (PartDif > 0.20 * max(abs(Signal1.RawData[i]), abs(Signal2.RawData[i])))
				return false;
		}

		return true;
	}

	return false;
}

IRProto* IRLib::GetProtocol() {
	for (auto& Protocol : IRLib::Protocols)
		if (Protocol->IsProtocol(RawData))
			return Protocol;

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

void IRLib::FillFromProntoHex(string &SrcString) {
	if (SrcString.size() < 20) {
		ESP_LOGE(tag, "Too small ProntoHEX data. Skipped");
		return;
	}

	//uint8_t	Learned = Converter::UintFromHexString<uint8_t>(SrcString.substr(0,4));
	Frequency = (uint16_t)(1000000/((Converter::UintFromHexString<uint16_t>(SrcString.substr(4,4)))* 0.241246));

	uint8_t OneTimeBurstLength 		= Converter::UintFromHexString<uint8_t>(SrcString.substr(8,4));
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
