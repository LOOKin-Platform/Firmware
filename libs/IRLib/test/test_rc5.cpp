/*

	vector<int32_t> Input1 = {+1778,-889,+889,-1778,+889,-889,+889,-889,+1778,-1778,+889,-889,+889,-889,+1778,-889,+889,-1778,+889,-889,+889,-45000};
	IRLib Signal1(Input1);
	ESP_LOGE("Signal1", "Protocol: %02X, Data %08X, IsRepeated: %s, RepeatPause: %d", Signal1.Protocol, Signal1.Uint32Data, (Signal1.IsRepeated) ? "true" : "false", Signal1.RepeatPause);
	vector<int32_t> Output1 = Signal1.GetRawDataForSending();
	bool Flag1 = IRLib::CompareIsIdentical(Input1, Output1);
	ESP_LOGE("Signal1 passed", "%s", (Flag1) ? "true" : "false");

	string SRC = "";
	for (int32_t& i : Signal1.GetRawDataForSending())
		SRC += (Converter::ToString<int32_t>(i) + " ");
	ESP_LOGE("Signal1 output", "%s", SRC.c_str());


	vector<int32_t> Input2 = {+889,-889,+1778,-1778,+1778,-889,+889,-1778,+1778,-889,+889,-1778,+889,-889,+1778,-1778,+889,-889,+889,-45000};
	IRLib Signal2(Input2);
	ESP_LOGE("Signal2", "Protocol: %02X, Data %08X, IsRepeated: %s, RepeatPause: %d", Signal2.Protocol, Signal2.Uint32Data, (Signal2.IsRepeated) ? "true" : "false", Signal2.RepeatPause);
	vector<int32_t> Output2 = Signal2.GetRawDataForSending();
	bool Flag2 = IRLib::CompareIsIdentical(Input2, Output2);
	ESP_LOGE("Signal2 passed", "%s", (Flag2) ? "true" : "false");

	vector<int32_t> Input3 = {+889,-889,+1778,-1778,+1778,-889,+889,-1778,+1778,-889,+889,-1778,+889,-889,+1778,-1778,+889,-889,+889,-45000};
	IRLib Signal3(Input3);
	ESP_LOGE("Signal3", "Protocol: %02X, Data %08X, IsRepeated: %s, RepeatPause: %d", Signal3.Protocol, Signal3.Uint32Data, (Signal3.IsRepeated) ? "true" : "false", Signal2.RepeatPause);
	vector<int32_t> Output3 = Signal3.GetRawDataForSending();
	bool Flag3 = IRLib::CompareIsIdentical(Input3, Output3);
	ESP_LOGE("Signal3 passed", "%s", (Flag3) ? "true" : "false");

	FreeRTOS::Sleep(5000);

	
*/
