#include "Unity.h"

#include <string>
#include <vector>
#include "Converter.h"
#include "IRLib.h"

using namespace std;

extern "C" 
{
	TEST_CASE("AiwaOperand", "IRLib")
	{
		string StringOperand = "9000 -4367 600 -467 567 -1600 600 -467 567 -1600 600 -1600 567 -467 600 -467 567 -467 600 -1600 600 -433 600 -467 600 -433 633 -433 600 -1567 600 -467 633 -1533 600 -467 600 -467 567 -1600 600 -1567 633 -1567 600 -467 567 -1600 600 -1600 567 -1600 600 -1600 600 -1567 600 -1600 567 -467 600 -1600 567 -1600 633 -433 567 -1600 600 -1600 600 -433 600 -467 600 -1567 600 -467 600 -433 600 -1600 600 -433 633 -433 600 -45000";
		vector<int32_t> IRVector;

		while (StringOperand.size() > 0)
		{
			size_t Pos = StringOperand.find(" ");

			string Item = (Pos != string::npos) ? StringOperand.substr(0,Pos) : StringOperand;
			int32_t IntItem = Converter::ToInt32(Item);

			IRVector.push_back(IntItem);

			if (Pos == string::npos || StringOperand.size() < Pos)
				StringOperand = "";
			else
				StringOperand.erase(0, Pos+1);
		}

		IRLib Signal(IRVector);

    	TEST_ASSERT_EQUAL(20, Signal.Protocol);
    	TEST_ASSERT_EQUAL(0xDB2408D7, Signal.Uint32Data);
    	TEST_ASSERT_EQUAL(0x02E5, Signal.MiscData);
	}
}