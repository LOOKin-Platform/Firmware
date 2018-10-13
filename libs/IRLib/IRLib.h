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

#include "Protocol.h"

using namespace std;

class IRLib {
	public:
		uint8_t 		Protocol 	= 0xF1;
		uint16_t 		Frequency 	= 38000;
		uint32_t 		Uint32Data 	= 0;
		vector<int32_t> RawData 	= vector<int32_t>();

		IRLib(string ProntoHex);
		IRLib(vector<int32_t> Raw	= vector<int32_t>());

		void 			LoadDataFromRaw();
		void 			FillRawData();
		string 			GetProntoHex();
		string			GetRawSignal();

		void 			SetFrequency(uint16_t);

		static bool		CompareIsIdentical(IRLib &Signal1, IRLib &Signal2);

	private:
		uint8_t		 	ProntoOneTimeBurst = 0;
		uint8_t 		ProntoRepeatBurst = 0;

		IRProto*		GetProtocol();
		IRProto*		GetProtocolByID(uint8_t);

		bool 			IsProntoHex();
		void 			FillFromProntoHex(string);
		string 			ProntoHexConstruct();

		static vector<IRProto *> Protocols;
};

#endif
