/*
 *    Protocol.h
 *    Template class for protocol subclassing
 *
 */

#ifndef LIBS_IR_PROTOCOL_H
#define LIBS_IR_PROTOCOL_H

#include "Converter.h"
#include "ACOperand.cpp"
#include "Settings.h"

#include <algorithm>

#define IR_PROTOCOL_RAW 0xFF

class IRProto {
	public:
		uint8_t							ID 					= 0xFF;
		uint16_t						DefinedFrequency 	= 38500;
		string 							Name 				= "";
		virtual bool					IsProtocol(vector<int32_t> &, uint16_t Start, uint16_t Length) 	{ return false; }
		virtual pair<uint32_t,uint16_t>	GetData(vector<int32_t>)										{ return make_pair(0x0,0x0);}
		virtual vector<int32_t> 		ConstructRaw(uint32_t Data, uint16_t Misc)						{ return vector<int32_t>(); }

		virtual vector<int32_t> 		ConstructRawRepeatSignal(uint32_t Data, uint16_t Misc)			{ return vector<int32_t>(); }
		virtual vector<int32_t> 		ConstructRawForSending(uint32_t Data, uint16_t Misc)			{ return vector<int32_t>(); }

		virtual int32_t					GetBlocksDelimeter()											{ return -Settings.SensorsConfig.IR.SignalEndingLen;}

		static bool TestValue(int32_t Value, int32_t Reference, float ComparisonDiff = 0.3) {
			if (Converter::Sign(Value) != Converter::Sign(Reference))
				return false;

			if (Value > 0)
				return (Value > (1-ComparisonDiff)*Reference && Value < (1+ComparisonDiff)*Reference) ? true : false;
			else
				return (Value > (1+ComparisonDiff)*Reference && Value < (1-ComparisonDiff)*Reference) ? true : false;
		}
};

#include "protocols/NEC1.cpp"
#include "Protocols/SONY.cpp"
#include "Protocols/NECx.cpp"
#include "protocols/Panasonic.cpp"
#include "protocols/Samsung36.cpp"
#include "protocols/RC5.cpp"
#include "Protocols/RC6.cpp"
//#include "protocols/Daikin.cpp"
//#include "protocols/MitsubishiAC.cpp"
//#include "protocols/Aiwa.cpp"
//#include "protocols/Gree.cpp"
//#include "protocols/HaierAC.cpp"
#endif
