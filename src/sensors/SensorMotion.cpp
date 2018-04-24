/*
*    SensorMotion.cpp
*    SensorMotion_t implementation
*
*/

static uint8_t 			SensorMotionID 		= 0xE1;
static adc1_channel_t 	SensorMotionChannel	= ADC1_CHANNEL_0;

static const adc_atten_t	atten = ADC_ATTEN_11db;

static void MotionDetectTask(void *) {
	while (1)  {
		uint16_t Value = (uint16_t)adc1_get_raw(SensorMotionChannel);

		if (Value > 2000) {
			Sensor_t::GetSensorByID(SensorMotionID)->SetValue(1);
			WebServer->UDPSendBroadcastUpdated(SensorMotionID, Converter::ToString(Sensor_t::GetSensorByID(SensorMotionID)->GetValue().Value));
			Automation->SensorChanged(SensorMotionID);

			while (Value > 2000) {
				Value = (uint16_t)adc1_get_raw(SensorMotionChannel);
				vTaskDelay(MOTION_GET_INTERVAL);
			}

			Sensor_t::GetSensorByID(SensorMotionID)->SetValue(0);
		}
		else {
			vTaskDelay(MOTION_GET_INTERVAL);
		}
	}
}

class SensorMotion_t : public Sensor_t {
  public:
    SensorMotion_t() {
        ID          = SensorMotionID;
        Name        = "Motion";
        EventCodes  = { 0x00, 0x01 };

	    switch (GetDeviceTypeHex()) {
	    		case DEVICE_TYPE_MOTION_HEX:
	    			SensorMotionChannel = MOTION_MOTION_CHANNEL; break;
	    		default:
	    			SensorMotionChannel = ADC1_CHANNEL_0;
	    }

	    if (SensorMotionChannel != ADC1_CHANNEL_0) {
	        adc1_config_width(ADC_WIDTH_BIT_12);
	        adc1_config_channel_atten(SensorMotionChannel, atten);
	        FreeRTOS::StartTask(MotionDetectTask, "MotionDetectTask", NULL, 4096);
	    }
    }

    void Update() override {
    		if (SetValue(ReceiveValue())) {
    			WebServer->UDPSendBroadcastUpdated(ID, Converter::ToString(GetValue().Value));
    			Automation->SensorChanged(ID);
    		}
    };

    double ReceiveValue(string Key = "Primary") override {
    		return ((uint16_t)adc1_get_raw(SensorMotionChannel) > 2000) ? 1 : 0;
    };

    bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
    		SensorValueItem ValueItem = GetValue();

    		if (SceneEventCode == 0x01 && ValueItem.Value == 1) return true;

    		return false;
    }
};
