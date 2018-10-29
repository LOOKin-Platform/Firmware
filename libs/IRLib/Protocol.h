/*
 *    Protocol.h
 *    Template class for protocol subclassing
 *
 */

#ifndef LIBS_IR_PROTOCOL_H
#define LIBS_IR_PROTOCOL_H

#define IR_PROTOCOL_RAW 0xFF

class IRProto {
	public:
		uint8_t					ID = 0x00;
		virtual bool			IsProtocol(vector<int32_t>) { return false; }
		virtual uint32_t		GetData(vector<int32_t>)	{ return 0x00; 	}
		virtual vector<int32_t> ConstructRaw(uint32_t Data) { return vector<int32_t>(); }
};

#include "protocols/NEC1.cpp"

#endif
