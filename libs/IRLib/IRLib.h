/*
 *    IRlib.h
 *    Class resolve different IR signals
 *
 */

#ifndef LIBS_IR_H
#define LIBS_IR_H

#include <vector>
#include <bitset>

#include <Converter.h>

using namespace std;

class IRLib {
	public:
		enum ProtocolEnum {
			NEC = 0x01, RAW = 0xF1
		};

		ProtocolEnum 	Protocol 	= RAW;
		uint16_t 		Frequency 	= 38000;
		uint32_t 		Uint32Data 	= 0;
		vector<int32_t> RawData 	= vector<int32_t>();

		IRLib(string ProntoHex);
		IRLib(vector<int32_t> Raw = vector<int32_t>());

		void 			LoadDataFromRaw();
		void 			FillRawData();
		string 			GetProntoHex();

		void 			SetFrequency(uint16_t);

	private:
		uint8_t		 	ProntoOneTimeBurst = 0;
		uint8_t 		ProntoRepeatBurst = 0;

		ProtocolEnum GetProtocol();

		bool 			IsNEC();
		uint32_t 		NECData();
		vector<int32_t> NECConstruct();

		bool 			IsProntoHex();
		void 			FillFromProntoHex(string);
		string 			ProntoHexConstruct();
};

#endif
