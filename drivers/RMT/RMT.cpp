/*
*    RMT.cpp
*    Class to using IR Remote Control module
*
*/

#include "RMT.h"

static char tag[] = "RMT";

/**
 * @brief Firstly init RMT driver
 *
 */

void RMT::Init() {
	if (IsInited)
		return;

	IsInited = true;
}

void RMT::Deinit() {
	if (!IsInited)
		return;
}

/**
 * @brief Set RX channel using for receiving data.
 *
 * @param [in] Pin The GPIO pin on which the signal is sent/received.
 * @param [in] Channel The RMT channel to work with.
 * @param [in] Callback Function to handle received data.
 */

void RMT::SetRXChannel(gpio_num_t Pin, rmt_channel_t Channel, IRChannelCallbackStart CallbackStart,
							IRChannelCallbackBody CallbackBody, IRChannelCallbackEnd CallbackEnd) {
	Init();

	rmt_config_t config					= RMT_DEFAULT_CONFIG_RX(Pin, Channel);
	config.rmt_mode                  	= RMT_MODE_RX;
	config.channel                   	= Channel;
	config.gpio_num                  	= Pin;
	config.mem_block_num             	= 6;
	config.clk_div                   	= RMT_CLK_DIV;
	config.rx_config.filter_en 			= true;
	config.rx_config.filter_ticks_thresh= 80;
	config.rx_config.idle_threshold 	= rmt_item32_TIMEOUT_US / 10 * (RMT_TICK_10_US);

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(Channel, 2500, 0));

#if CONFIG_IDF_TARGET_ESP32
	ESP_ERROR_CHECK(rmt_set_source_clk(Channel, RMT_BASECLK_REF));
#elif CONFIG_IDF_TARGET_ESP32C6
	ESP_ERROR_CHECK(rmt_set_source_clk(Channel, RMT_BASECLK_XTAL));
#endif

	ChannelsMap[Channel].Pin 			= Pin;
	ChannelsMap[Channel].CallbackStart 	= CallbackStart;
	ChannelsMap[Channel].CallbackBody 	= CallbackBody;
	ChannelsMap[Channel].CallbackEnd 	= CallbackEnd;

	::rmt_rx_memory_reset(Channel);

	//rtc_gpio_isolate(Pin);
}

/**
 * @brief Start receiving for given channel.
 *
 * @param [in] Channel The RMT channel to work with.
 *
 */
void RMT::ReceiveStart(rmt_channel_t Channel) {
	if (RMT::ChannelsMap.count(Channel) > 0)
		if (RMT::ChannelsMap[Channel].Handle != NULL) {
			ESP_LOGE(tag,"Channel task already started");
			return;
		}

	RMT::ChannelsMap[Channel].Handle = FreeRTOS::StartTask(RXTask, "RXReadTask" + static_cast<int>(Channel), (void *)(uint32_t)static_cast<int>(Channel), 4096);

	if (RMT::ChannelsMap[Channel].Handle == NULL)
		ESP_LOGE("RMT::ReceiveStart", "NULL HANDLE");
}


/**
 * @brief Stop receiving for given channel.
 *
 * @param [in] Channel The RMT channel to work with.
 *
 */
void RMT::ReceiveStop(rmt_channel_t Channel) {
	ESP_ERROR_CHECK(::rmt_rx_stop(Channel));

	if (RMT::ChannelsMap.count(Channel) > 0)
		if (RMT::ChannelsMap[Channel].Handle == 0 || RMT::ChannelsMap[Channel].Handle == NULL) {
			ESP_LOGE(tag,"Channel task doesn't exist");
			return;
		}

	FreeRTOS::DeleteTask(RMT::ChannelsMap[Channel].Handle);
	RMT::ChannelsMap[Channel].Handle = 0;
}

/**
 * @brief Completely stop RX and uninstall RX driver.
 *
 * @param [in] Channel The RMT channel to work with.
 *
 */
void RMT::UnsetRXChannel(rmt_channel_t Channel) {
	ReceiveStop(Channel);
	rmt_driver_uninstall(Channel);
}

void RMT::RXTask(void *TaskData) {
	uint32_t ChannelNum = (uint32_t)TaskData;

    if (ChannelNum != RMT_CHANNEL_2) {
    	ESP_LOGE(tag, "Channel number out of range");
    	return;
    }

    rmt_channel_t Channel = static_cast<rmt_channel_t>(ChannelNum);

	RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle(Channel, &rb);

	ESP_ERROR_CHECK(::rmt_rx_start(Channel , 1));

    while(rb) {
		size_t rx_size = 0;
		rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 2500);

		if(item)
		{
			int offset = 0;

			if (ChannelsMap[Channel].CallbackStart != nullptr)
				ChannelsMap[Channel].CallbackStart();

			uint16_t SignalSize = rx_size;

			while (offset < SignalSize)
			{
				if (ChannelsMap[Channel].CallbackBody != nullptr) {
					if (!ChannelsMap[Channel].CallbackBody(PrepareBit((bool)(item+offset)->level0, (item+offset)->duration0)))
						break;

					if (!ChannelsMap[Channel].CallbackBody(PrepareBit((bool)(item+offset)->level1, (item+offset)->duration1)))
						break;
				}
				offset++;
			}

			//after parsing the data, return spaces to ringbuffer.
			vRingbufferReturnItem(rb, (void*) item);

			if (ChannelsMap[Channel].CallbackEnd != nullptr)
				ChannelsMap[Channel].CallbackEnd();
		}
	}
	vTaskDelete(NULL);
}


/**
 * @brief Set TX channel using for sending data.
 *
 * @param [in] Pin The GPIO pin on which the signal is sent/received.
 * @param [in] Channel The RMT channel to work with.
 * @param [in] Frequent output Frequent for IR signal.
 */

void RMT::SetTXChannel(vector<gpio_num_t> GPIO, rmt_channel_t Channel, uint16_t Frequency) {
	Init();

	if (GPIO.size() == 0)
		return;

	ESP_LOGI("SetTXChannel", "Frequency: %d", Frequency);

	bool IsAllEmpty = true;

	for(auto& GPIOItem : GPIO)
		if (GPIOItem != GPIO_NUM_0)
		{
			IsAllEmpty = false;
			break;
		}

	if (IsAllEmpty)
		return;

	rmt_config_t config					= RMT_DEFAULT_CONFIG_TX(GPIO[0], Channel);
	config.rmt_mode                  	= RMT_MODE_TX;
	config.channel                   	= Channel;
	config.gpio_num                  	= GPIO[0]; //GPIO_NUM_4
	config.mem_block_num             	= 6;
	config.clk_div                   	= 80;
	config.tx_config.loop_en         	= false;
	config.tx_config.carrier_en      	= true;
	config.tx_config.idle_output_en  	= true;
	config.tx_config.idle_level      	= (rmt_idle_level_t)0;
	config.tx_config.carrier_freq_hz 	= Frequency;
	config.tx_config.carrier_level   	= (rmt_carrier_level_t)1;
	config.tx_config.carrier_duty_percent = 50;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(Channel, 0, 0));
	//ESP_ERROR_CHECK(rmt_set_source_clk(Channel, RMT_BASECLK_APB));

	for (int i = 1; i < GPIO.size(); i++) {
		if (GPIO[i] != GPIO_NUM_0)
			rmt_set_gpio(Channel, RMT_MODE_TX, GPIO[i], false);
	}

	/*
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_36);
	 */

	ChannelsMap[Channel].Pin 			= GPIO[0];
	ChannelsMap[Channel].Frequency 		= Frequency;

	 //::rtc_gpio_isolate(Pin);
}

/**
 * @brief Completely unset TX channel
 *
 * @param [in] Channel The RMT channel to work with.
 */

void RMT::UnsetTXChannel(rmt_channel_t Channel) {
	rmt_tx_stop(Channel);
	rmt_driver_uninstall(Channel);
}

/**
 * @brief Wrapper for TAddItem function
 *
 * @param [in] Bit Bit to add to the queue. Negative means 0, positive - 1. Modul means duration of the bit
 */
void IRAM_ATTR RMT::TXAddItem(int32_t Bit) {
	//ESP_LOGE("TXADDItem", "%d", Bit);

	// hack for large pauses between signals
	if (abs(Bit) >= Settings.SensorsConfig.IR.Threshold)// && abs(Bit) != Settings.SensorsConfig.IR.SignalEndingLen)
	{
		uint8_t Parts = (uint8_t)ceil(abs(Bit) / (double)Settings.SensorsConfig.IR.Threshold);

		for (int i = 0; i < Parts; i++)
			TXAddItemExact((int32_t)floor(Bit/Parts));
	}
	else
		TXAddItemExact(Bit);
}

/**
 * @brief Add a level/duration to the transaction to be written.
 *
 * @param [in] Bit Bit to add to the queue. Negative means 0, positive - 1. Modul means duration of the bit
 */
void IRAM_ATTR RMT::TXAddItemExact(int32_t Bit) {
	if (Bit == 0) return;

	if (OutputItemsSize == 0 || (OutputItemsSize > 0 && OutputItems[OutputItemsSize - 1].duration1 != 0))
	{
		rmt_item32_t Item;
		Item.level0 	= (Bit > 0 ) ? 1 : 0 ;
		Item.duration0 	= abs(Bit);
		Item.level1 	= 0;
		Item.duration1 	= 0;

		OutputItems[OutputItemsSize] = Item;

		OutputItemsSize++;
	}
	else
	{
		OutputItems[OutputItemsSize - 1].level1	= (Bit > 0 ) ? 1 : 0 ;
		OutputItems[OutputItemsSize - 1].duration1= abs(Bit);
	}
}

/**
 * @brief Set Items to be send by RMT.
 *
 * @param [in] Vector of int32_t items
 *
 */
void IRAM_ATTR RMT::TXSetItems(vector<int32_t> Items) {
	TXClear();

	for (int32_t &Item : Items)
		TXAddItem(Item);
}


/**
 * @brief Clear any previously written level/duration pairs that have not been sent.
 */
void RMT::TXClear() {
	OutputItems[0]  = {0};
	OutputItemsSize = 0;
}

/**
 * @brief Get items count ready for transmit.
 *
 * @param [out] Items in Queue
 *
 */
int16_t RMT::TXItemsCount() {
	int16_t ItemsCount = OutputItemsSize * 2;

	if (ItemsCount > 0)
		if (OutputItems[OutputItemsSize - 1].duration1 == 0 && OutputItems[OutputItemsSize - 1].level1)
			ItemsCount--;

	return ItemsCount;
}

/**
 * @brief Send the items out through the RMT.
 *
 * The level/duration set of bits that were added to the transaction are written
 * out through the RMT device.  After transmission, the list of level/durations
 * is cleared.
 *
 * @param [in] Channel Channel to send data
 *
 */



void IRAM_ATTR RMT::TXSend(rmt_channel_t Channel) {
	//if (OutputItems == 0) return;

	if (OutputItems[OutputItemsSize - 1].duration0 < 30000 || OutputItems[OutputItemsSize - 1].duration1 < 30000)
		TXAddItem(-Settings.SensorsConfig.IR.SignalEndingLen);

	OutputItems[OutputItemsSize++] = { 0 };

	/*
	for (int i = 0; i < OutputItemsSize; i++) {
		string Output = (OutputItems[i].level0 ? "+" : "-") + Converter::ToString<uint16_t>(OutputItems[i].duration0);
		ESP_LOGE("ITEM", "%s", Output.c_str());
		Output = (OutputItems[i].level1 ? "+" : "-") + Converter::ToString<uint16_t>(OutputItems[i].duration1);
		ESP_LOGE("ITEM", "%s", Output.c_str());
	}
	*/

	rmt_write_items(Channel, OutputItems,  OutputItemsSize, true);

    //::rmt_fill_tx_items(Channel, morse_esp, sizeof(morse_esp) / sizeof(morse_esp[0]), 0);
	//::rmt_fill_tx_items(Channel, &OutputItems[0], OutputItems.size(), 0);
	//::rmt_tx_start(Channel, true);
	//::rmt_wait_tx_done(Channel, portMAX_DELAY);

	TXClear();
}

/**
 * @brief Convert IRBit to int16_t.
 *
 * @param [in] Bit 1 or 0 received bit.
 * @param [in] Interval signal duration.
 *
 * @return int16_t bit representation.
 *
 */
int32_t IRAM_ATTR RMT::PrepareBit(bool Bit, uint32_t Interval) {
	Interval = round(Interval / 10) * 10;
	return (!Bit) ? Interval : -Interval;
}
