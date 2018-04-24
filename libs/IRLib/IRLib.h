/*
*    IRlib.h
*    Class resolve different IR signals
*
*/
using namespace std;

#include <vector>
#include <bitset>

#include <Converter.h>

class IRLib {
	public:
		enum ProtocolEnum { UNKNOWN = 0x00, NEC = 0x01 };
		ProtocolEnum 	Protocol 		= UNKNOWN;
		uint16_t			Frequency 		= 38000;
		uint32_t			Uint32Data 		= 0;
		vector<int32_t>	RawData 			= vector<int32_t>();

		IRLib(string ProntoHex);
		IRLib(vector<int32_t> Raw = vector<int32_t>());

		void 			LoadDataFromRaw();
		void 			FillRawData();
		string 			GetProntoHex();

		void 			SetFrequency(uint16_t);

	private:
		uint8_t			ProntoOneTimeBurst 	= 0;
		uint8_t			ProntoRepeatBurst 	= 0;

		ProtocolEnum 	GetProtocol();

		bool 			IsNEC();
		uint32_t			NECData();
		vector<int32_t>	NECConstruct();

		bool 			IsProntoHex();
		void 			FillFromProntoHex(string);
		string			ProntoHexConstruct();
};
