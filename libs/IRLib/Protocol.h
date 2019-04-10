/*
 *    Protocol.h
 *    Template class for protocol subclassing
 *
 */

#ifndef LIBS_IR_PROTOCOL_H
#define LIBS_IR_PROTOCOL_H

#include "Converter.h"

#define IR_PROTOCOL_RAW 0xFF

class IRProto {
	public:
		uint8_t					ID 					= 0x00;
		uint16_t				DefinedFrequency 	= 38500;
		string 					Name 				= "";
		virtual bool			IsProtocol(vector<int32_t>) 				{ return false; }
		virtual uint32_t		GetData(vector<int32_t>)					{ return 0x00; 	}
		virtual vector<int32_t> ConstructRaw(uint32_t Data) 				{ return vector<int32_t>(); }

		virtual vector<int32_t> ConstructRawRepeatSignal(uint32_t Data) 	{ return vector<int32_t>(); }
		virtual vector<int32_t> ConstructRawForSending(uint32_t Data)		{ return vector<int32_t>(); }

		bool TestValue(int32_t Value, int32_t Reference) {
			if (Converter::Sign(Value) != Converter::Sign(Reference))
				return false;

			if (Value > 0)
				return (Value > 0.85*Reference && Value < 1.15*Reference) ? true : false;
			else
				return (Value > 1.15*Reference && Value < 0.85*Reference) ? true : false;
		}
};

#include "protocols/NEC1.cpp"
#include "protocols/SONY_SIRC.cpp"
#include "protocols/Samsung.cpp"

#endif
