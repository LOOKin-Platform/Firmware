/*
*    SensorIR.cpp
*    SensorIR_t implementation
*
*/

#include <IRLib.h>

static vector<int32_t> 	SensorIRCurrentMessage 			= {};
static uint8_t 			SensorIRID 						= 0x87;
static gpio_num_t 		SensorIRFrequencyDetectorGPIO 	= GPIO_NUM_0;
static QueueHandle_t 	SensorIRQueueFrequencyDetect	= FreeRTOS::Queue::Create(20, sizeof(uint32_t));

static void IRAM_ATTR FrequencyDetectHandler(void *args) {
	if (!FreeRTOS::Queue::IsQueueFullFromISR(SensorIRQueueFrequencyDetect)) {
	 	uint32_t CurrentTime = Time::UptimeU();
	 	FreeRTOS::Queue::SendToBackFromISR(SensorIRQueueFrequencyDetect, &CurrentTime);
	}
}

class SensorIR_t : public Sensor_t {
  public:
	SensorIR_t() {
	    ID          = SensorIRID;
	    Name        = "IR";
	    EventCodes  = { 0x00, 0x01 };

	    switch (GetDeviceTypeHex()) {
	    	case Settings.Devices.Remote:
				if (IR_REMOTE_RECEIVER_GPIO != GPIO_NUM_0) {
					RMT::SetRXChannel(IR_REMOTE_RECEIVER_GPIO, RMT_CHANNEL_0, SensorIR_t::MessageStart, SensorIR_t::MessageBody, SensorIR_t::MessageEnd);
					RMT::ReceiveStart(RMT_CHANNEL_0);
				}

	    		if (IR_REMOTE_DETECTOR_PIN != GPIO_NUM_0)
	    			FrequencyDetectInit(IR_REMOTE_DETECTOR_PIN);

	    		break;
	    }
	}

    void Update() override {};

    double ReceiveValue(string = "Primary") override {
    	return 0;
    };

    string FormatValue(string Key = "Primary") override {
    	return Converter::ToHexString(Values[Key].Value, 8);
    }

    bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
    	// test data
		SensorValueItem ValueItem = GetValue();

		if (SceneEventCode == 0x01 && ValueItem.Value == 0x1) return true;
		if (SceneEventCode == 0xEE && ValueItem.Value == 0xEE) return true;

		return false;
    };

    static void MessageStart() {
    	SensorIRCurrentMessage.clear();
    };

    static void MessageBody(int16_t Bit) {
    	SensorIRCurrentMessage.push_back(Bit);
    };

    static void MessageEnd() {
    	if (SensorIRCurrentMessage.size() <= 8) {
    		FreeRTOS::Queue::Reset(SensorIRQueueFrequencyDetect);
    		return;
    	}

    	IRLib IRSignal(SensorIRCurrentMessage);
    	IRSignal.SetFrequency(FrequencyDetectCalculate());

		ESP_LOGI("IRSensor_t", "Calculated frequency: %s", Converter::ToString(IRSignal.Frequency).c_str());

    	if (IRSignal.Protocol == IRLib::ProtocolEnum::NEC) {
    		Sensor_t::GetSensorByID(SensorIRID)->SetValue(0x01);
    		Sensor_t::GetSensorByID(SensorIRID)->SetValue(IRSignal.Uint32Data,"01");

    		ESP_LOGI("IRSensor_t", "NEC signal received. %s", Converter::ToHexString(IRSignal.Uint32Data,2).c_str());
    		return;
    	}
    	else {
    		Sensor_t::GetSensorByID(SensorIRID)->SetValue(0xEE);
    	}

    	WebServer.UDPSendBroadcastUpdated(SensorIRID, Converter::ToHexString(Sensor_t::GetSensorByID(SensorIRID)->GetValue().Value,2));
    	Automation.SensorChanged(SensorIRID);

    	ESP_LOGI("IRSensor_t", "Raw signal received");
    };

    static void FrequencyDetectInit(gpio_num_t GPIO) {
    	if (GPIO != GPIO_NUM_0) {
    		SensorIRFrequencyDetectorGPIO = GPIO;

    		gpio_config_t io_conf;
    		io_conf.pin_bit_mask 	= 1ULL<<SensorIRFrequencyDetectorGPIO;
    		io_conf.mode 			= GPIO_MODE_INPUT;
    		io_conf.pull_up_en 		= GPIO_PULLUP_DISABLE;
    		io_conf.pull_down_en 	= GPIO_PULLDOWN_DISABLE;
    		io_conf.intr_type 		= GPIO_INTR_ANYEDGE;
    		gpio_config(&io_conf);

    		::gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    		::gpio_isr_handler_add(SensorIRFrequencyDetectorGPIO, FrequencyDetectHandler, NULL);
    	}
    }

    static uint16_t FrequencyDetectCalculate() {
		if (IR_REMOTE_DETECTOR_PIN != GPIO_NUM_0) {
			vector<uint16_t> Intervals;
			uint32_t	 MeasuredTime = 0;

			while (FreeRTOS::Queue::Count(SensorIRQueueFrequencyDetect)) {
				uint32_t OldTime = MeasuredTime;
				FreeRTOS::Queue::Receive(SensorIRQueueFrequencyDetect, &MeasuredTime);

				if (OldTime > 0) Intervals.push_back(MeasuredTime	- OldTime);
			}

			sort(Intervals.begin(), Intervals.end());

			if (Intervals.size() > 5) {
				Intervals.erase(Intervals.begin() + round(0.6*Intervals.size()));

				float	 IntervalsSum = 0;
				for(auto &i : Intervals)
					IntervalsSum += i;

				return round(1000000/(IntervalsSum/Intervals.size()));
			}
			else
				return 0;
		}
    }
};
