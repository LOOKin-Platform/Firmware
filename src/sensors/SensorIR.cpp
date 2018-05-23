/*
*    SensorIR.cpp
*    SensorIR_t implementation
*
*/

#include <IRLib.h>
#include <ISR.h>

static vector<int32_t> 		SensorIRCurrentMessage 			= {};
static uint8_t 				SensorIRID 						= 0x87;
static gpio_num_t 			SensorIRFrequencyDetectorGPIO 	= GPIO_NUM_0;
static constexpr uint8_t 	SensorIRQueueSize				= 30;
static QueueHandle_t 		SensorIRQueueFrequencyDetect	= FreeRTOS::Queue::Create(SensorIRQueueSize, sizeof(uint32_t));

static bool Previous = false;

static IRLib 	LastSignal;
static uint32_t SignalDetectionTime = 0;

static void IRAM_ATTR FrequencyDetectHandler(void *args) {
	/*
	uint8_t ItemsInQueue = FreeRTOS::Queue::CountFromISR(SensorIRQueueFrequencyDetect);

	if (ItemsInQueue < SensorIRQueueSize) {
		bool Value = GPIO::Read(SensorIRFrequencyDetectorGPIO);

		if (ItemsInQueue == 0 && Value) {
			Previous = true;
			return;
		}

		uint32_t CurrentTime = Time::UptimeU();

		if (Previous && !Value) {
			Previous = false;
		 	FreeRTOS::Queue::SendToBackFromISR(SensorIRQueueFrequencyDetect, &CurrentTime);
		}

		if (!Previous && Value) {
			Previous = true;
		 	FreeRTOS::Queue::SendToBackFromISR(SensorIRQueueFrequencyDetect, &CurrentTime);
		}
	}
	*/
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

    void Update() override {
    	Values.clear();

    	if (LastSignal.Protocol == IRLib::ProtocolEnum::RAW && LastSignal.RawData.size() == 0)
    	{
    		SetValue(0);
    		return;
    	}

    	Values["Primary"]	= SensorValueItem(static_cast<uint8_t>(LastSignal.Protocol), SignalDetectionTime);
    	Values["Signal"]	= SensorValueItem(LastSignal.Uint32Data, SignalDetectionTime);
    	Values["Frequency"] = SensorValueItem(LastSignal.Frequency, SignalDetectionTime);
    };

    string FormatValue(string Key = "Primary") override {
    	if (Key == "Primary")
        	return Converter::ToHexString(Values[Key].Value, 2);

    	if (Key == "Signal" && LastSignal.Protocol == IRLib::ProtocolEnum::RAW && LastSignal.RawData.size() != 0) {
    		string SignalOutput = "";

    		for (int i=0; i < LastSignal.RawData.size(); i++)
    			SignalOutput += Converter::ToString(LastSignal.RawData[i]) + ((i != LastSignal.RawData.size() - 1) ? " " : "");

    		return SignalOutput;
    	}

    	if (Key == "Frequency")
    		return Converter::ToString(LastSignal.Frequency);

    	return Converter::ToHexString(Values[Key].Value, 8);
    }

    bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override {
    	// test data
		SensorValueItem ValueItem = GetValue();

		if (SceneEventCode == 0x01 && ValueItem.Value == 0x1) return true;
		if (SceneEventCode == 0xF1 && ValueItem.Value == 0xF1) return true;

		return false;
    };

    static void MessageStart() {
    	SensorIRCurrentMessage.clear();
    };

    static void MessageBody(int16_t Bit) {
    	if (SensorIRCurrentMessage.size() > 0 && SensorIRCurrentMessage.back() < -40000)
    		return;

    	if (Bit == 0) SensorIRCurrentMessage.push_back(-45000);
    	else SensorIRCurrentMessage.push_back(Bit);
    };

    static void MessageEnd() {
    	if (SensorIRCurrentMessage.size() <= 8) {
    		FreeRTOS::Queue::Reset(SensorIRQueueFrequencyDetect);
    		return;
    	}

    	LastSignal = IRLib(SensorIRCurrentMessage);
    	SignalDetectionTime = Time::Unixtime();
    	LastSignal.SetFrequency(FrequencyDetectCalculate());

    	SensorIRCurrentMessage.empty();

    	WebServer.UDPSendBroadcastUpdated(SensorIRID, Converter::ToHexString(static_cast<uint8_t>(LastSignal.Protocol),2));
    	Automation.SensorChanged(SensorIRID);
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

			while (FreeRTOS::Queue::Count(SensorIRQueueFrequencyDetect) > 2) {
				uint32_t StartTime 	= 0;
				uint32_t EndTime 	= 0;
				FreeRTOS::Queue::Receive(SensorIRQueueFrequencyDetect, &StartTime);
				FreeRTOS::Queue::Receive(SensorIRQueueFrequencyDetect, &EndTime);

				Intervals.push_back(EndTime	- StartTime);
			}

			FreeRTOS::Queue::Reset(SensorIRQueueFrequencyDetect);

			if (Intervals.size() > 10) {
				uint8_t IntervalsSize = Intervals.size();
				Intervals.erase(Intervals.begin() + round(0.8 * IntervalsSize), Intervals.end());
				Intervals.erase(Intervals.begin(), Intervals.begin() + round(0.2 * Intervals.size()));

				float IntervalsSum = 0;
				for(auto &i : Intervals)
					IntervalsSum += i;

				for (uint16_t Interval : Intervals)
					ESP_LOGI("Detector", "Interval %d", Interval);


				return round(1000000/(IntervalsSum/Intervals.size()));
			}
			else
				return 0;
		}
    }
};
