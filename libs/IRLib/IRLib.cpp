/*
*    IRlib.cpp
*    Class resolve different IR signals
*
*/

#include "IRLib.h"

static char tag[] = "IRLib";

vector<IRProto *> IRLib::Protocols = vector<IRProto *>();

IRLib::IRLib(vector<int32_t> Raw) {
	RawData = Raw;

	if (IRLib::Protocols.size() == 0)
		Protocols = { new NEC1() };

	LoadDataFromRaw();
}

IRLib::IRLib(string ProntoHex) {
	Converter::FindAndRemove(ProntoHex, " ");
	FillFromProntoHex(ProntoHex);
	LoadDataFromRaw();
}

void IRLib::LoadDataFromRaw() {
	IRProto *Protocol = GetProtocol();

	this->Protocol 		= 0x0;
	this->Uint32Data 	= 0x0;

	if (Protocol!=nullptr) {
		this->Protocol 	= Protocol->ID;
		this->Uint32Data= Protocol->GetData(RawData);
	}
}

void IRLib::FillRawData() {
	IRProto *Proto = GetProtocolByID(this->Protocol);

	if (Proto != nullptr)
		this->RawData = Proto->ConstructRaw(this->Uint32Data);
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

bool IRLib::CompareIsIdentical(IRLib &Signal1, IRLib &Signal2) {
	if (Signal1.Protocol == Signal2.Protocol) {
		uint16_t SizeDiff		= abs(Signal1.RawData.size() - Signal2.RawData.size());
		uint16_t MinimalSize	= min(Signal1.RawData.size(), Signal2.RawData.size());

		if (SizeDiff >= 5) // too big difference between signals length
			return false;

		for (uint16_t i=0; i< MinimalSize; i++) {
			uint16_t PartDif = abs(Signal1.RawData[i] - Signal2.RawData[i]);

			if (PartDif > 0.1 * max(abs(Signal1.RawData[i]), abs(Signal2.RawData[i])))
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

void IRLib::FillFromProntoHex(string SrcString) {
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
