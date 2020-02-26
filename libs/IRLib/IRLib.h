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
		uint8_t			Protocol	= 0xFF;
		uint16_t		Frequency	= 38000;
		uint32_t		Uint32Data	= 0;
		uint16_t		MiscData	= 0;

		bool			IsRepeated	= false;

		vector<int32_t> RawData 	= vector<int32_t>();

		IRLib(string &ProntoHex);
		IRLib(vector<string> 	Raw);
		IRLib(vector<int32_t> 	Raw	= vector<int32_t>());

		void 			LoadDataFromRaw();
		void 			FillRawData();
		string 			GetProntoHex();
		string			GetRawSignal();

		vector<int32_t>	GetRawDataForSending();
		vector<int32_t>	GetRawRepeatSignalForSending();
		uint16_t 		GetProtocolFrequency();

		void 			SetFrequency(uint16_t);

		void			ExternFillPostOperations();

		void			AppendRawSignal(IRLib &);
		int32_t			RawPopItem();

		static bool		CompareIsIdentical(IRLib &Signal1, IRLib &Signal2);

	private:
		uint8_t		 	ProntoOneTimeBurst 	= 0;
		uint8_t 		ProntoRepeatBurst 	= 0;

		void 			FillProtocols();

		IRProto*		GetProtocol();
		IRProto*		GetProtocolByID(uint8_t);

		bool 			IsProntoHex();
		void 			FillFromProntoHex(string &);
		string 			ProntoHexConstruct();

		static vector<IRProto *> Protocols;
};

#endif
