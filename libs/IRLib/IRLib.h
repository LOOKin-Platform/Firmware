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
#include <cstring>

using namespace std;

class IRLib {
	public:
		uint8_t			Protocol	= 0xFF;
		uint16_t		Frequency	= 38000;
		uint32_t		Uint32Data	= 0;
		uint16_t		MiscData	= 0;

		bool			IsRepeated	= false;
		uint8_t			RepeatCount	= 0;
		uint32_t		RepeatPause = 0;

		vector<int32_t> RawData 	= vector<int32_t>();

		IRLib(const char *);
		IRLib(string &ProntoHex);
		IRLib(int32_t *);
		IRLib(vector<string> 	Raw);
		IRLib(vector<int32_t> 	Raw	= vector<int32_t>());
		void 			LoadFromRawString(string &);

		void 			LoadDataFromRaw();
		void 			FillRawData();
		string 			GetProntoHex(bool SpaceDelimeter = true);
		string			GetRawSignal();
		string			GetSignalCRC();

		vector<int32_t> GetRawDataForSending();
		vector<int32_t>	GetRawRepeatSignalForSending();
		uint16_t 		GetProtocolFrequency();

		void 			SetFrequency(uint16_t);

		void			ExternFillPostOperations();

		void			AppendRawSignal(IRLib &);
		void			AppendRawSignal(vector<int32_t> &);
		int32_t			RawPopItem();

		bool			CompareIsIdenticalWith(IRLib &Signal, float ComparisonDiff = 0.25);
		bool			CompareIsIdenticalWith(vector<int32_t> &, float ComparisonDiff = 0.25);

		static bool		CompareIsIdentical(IRLib &Signal1, IRLib &Signal2);
		static bool		CompareIsIdentical(vector<int32_t> &, vector<int32_t> &, uint16_t Signal1Start = 0, uint16_t Signal2Start = 0, uint16_t Signal1Size = 0, uint16_t Signal2Size = 0, float ComparisonDiff = 0.25);
		static uint8_t 	GetProtocolExternal(vector<int32_t> &);

		static bool		TestSignal(vector<int32_t> Input, float ComparisonDiff = 0.25);
		static void		TestAll();

	private:
		uint8_t		 	ProntoOneTimeBurst 	= 0;
		uint8_t 		ProntoRepeatBurst 	= 0;

		static void		FillProtocols();

		IRProto*		GetProtocol(uint16_t Start = 0, uint16_t Length = 0);
		IRProto*		GetProtocolByID(uint8_t);

		bool 			IsProntoHex();
		void 			FillFromProntoHex(string &);
		void 			FillFromProntoHex(const char *);

		string 			ProntoHexConstruct(bool SpaceDelimeter = true);

		bool 			CRCCompareFunction(uint16_t Item1, uint16_t Item2, float Threshold = 0.3);

		bool 			IsSkipProntoFlag = false;

		static vector<IRProto *> Protocols;
};

#endif
