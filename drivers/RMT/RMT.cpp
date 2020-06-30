/*
*    RMT.cpp
*    Class to using IR Remote Control module
*
*/

#include "RMT.h"

static char tag[] = "RMT";


vector<rmt_item32_t>	RMT::OutputItems 		= {};
RMT::IRChannelInfo_t	RMT::ChannelInfo 		= RMT::IRChannelInfo_t();

/**
 * @brief Start receiving for IRSignals
 *
 * @param [in] Pin The GPIO pin on which the signal is sent/received.
 * @param [in] IRChannelCallbackStart 	Function to handle received data.
 * @param [in] IRChannelCallbackBody 	Function to handle received data.
 * @param [in] IRChannelCallbackEnd 	Function to handle received data.
 *
 */
void RMT::RxStart(gpio_num_t Pin, IRChannelCallbackStart CallbackStart, IRChannelCallbackBody CallbackBody, IRChannelCallbackEnd CallbackEnd) {

	if (ChannelInfo.RxHandle != NULL)
		return;

	rmt_config_t config;
	config.rmt_mode                  	= RMT_MODE_RX;
	config.channel                   	= RMT_CHANNEL_0;
	config.gpio_num                  	= Pin;
	config.mem_block_num             	= 8;
	config.clk_div                   	= RMT_CLK_DIV;
	config.rx_config.filter_en 			= true;
	config.rx_config.filter_ticks_thresh= 80;
	config.rx_config.idle_threshold 	= rmt_item32_TIMEOUT_US / 10 * (RMT_TICK_10_US);

	ESP_ERROR_CHECK(::rmt_config(&config));
	ESP_ERROR_CHECK(::rmt_driver_install(RMT_CHANNEL_0, 2500, 0));
	ESP_ERROR_CHECK(::rmt_set_source_clk(RMT_CHANNEL_0, RMT_BASECLK_REF));

	ChannelInfo.Pin 			= Pin;
	ChannelInfo.CallbackStart 	= CallbackStart;
	ChannelInfo.CallbackBody 	= CallbackBody;
	ChannelInfo.CallbackEnd 	= CallbackEnd;

	ChannelInfo.RxHandle = FreeRTOS::StartTask(RXTask, "RMTRxTask", NULL, 4096);
}


/**
 * @brief Stop receiving for given channel.
 *
 * @param [in] Channel The RMT channel to work with.
 *
 */
void RMT::RxStop(bool SaveData) {
	 ESP_ERROR_CHECK(::rmt_rx_stop(RMT_CHANNEL_0));

	 if (ChannelInfo.RxHandle == NULL) {
		 ESP_LOGE(tag,"Channel task doesn't exist");
		 return;
	 }

	 FreeRTOS::DeleteTask(ChannelInfo.RxHandle);
	 ChannelInfo.RxHandle = NULL;

	 if (!SaveData)
		 ChannelInfo = RMT::IRChannelInfo_t();

	 ::rmt_driver_uninstall(RMT_CHANNEL_0);
}

void RMT::RXTask(void *TaskData) {
	uint32_t ChannelNum = (uint32_t)TaskData;

    rmt_channel_t Channel = static_cast<rmt_channel_t>(ChannelNum);

	RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle(Channel, &rb);

	ESP_ERROR_CHECK(::rmt_rx_start(Channel , 1));

    while(rb) {
		size_t rx_size = 0;
		rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 1000);

		if(item)
		{
			int offset = 0;

			if (ChannelInfo.CallbackStart != nullptr)
				ChannelInfo.CallbackStart();

			uint16_t SignalSize = rx_size;

			while (offset < SignalSize)
			{
				if (ChannelInfo.CallbackBody != nullptr) {
					if (!ChannelInfo.CallbackBody(PrepareBit((bool)(item+offset)->level0, (item+offset)->duration0)))
						break;

					if (!ChannelInfo.CallbackBody(PrepareBit((bool)(item+offset)->level1, (item+offset)->duration1)))
						break;
				}
				offset++;
			}

			//after parsing the data, return spaces to ringbuffer.
			vRingbufferReturnItem(rb, (void*) item);

			if (ChannelInfo.CallbackEnd != nullptr)
				ChannelInfo.CallbackEnd();
		}
	}
	vTaskDelete(NULL);
}


/**
 * @brief Wrapper for TAddItem function
 *
 * @param [in] Bit Bit to add to the queue. Negative means 0, positive - 1. Modul means duration of the bit
 */
void RMT::TXAddItem(int32_t Bit) {
	// hack for large pauses between signals
	if (abs(Bit) >= Settings.SensorsConfig.IR.Threshold && abs(Bit) != Settings.SensorsConfig.IR.SignalEndingLen)
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
void RMT::TXAddItemExact(int32_t Bit) {
	if (Bit == 0) return;

	if (OutputItems.size() == 0 || (OutputItems.back()).duration1 != 0)
	{
		rmt_item32_t Item;
		Item.level0 	= (Bit > 0 ) ? 1 : 0 ;
		Item.duration0 	= abs(Bit);
		Item.level1 	= 0;
		Item.duration1 	= 0;

		OutputItems.push_back(Item);
	}
	else
	{
		OutputItems.back().level1	= (Bit > 0 ) ? 1 : 0 ;
		OutputItems.back().duration1= abs(Bit);
	}
}

/**
 * @brief Set Items to be send by RMT.
 *
 * @param [in] Vector of int32_t items
 *
 */
void RMT::TXSetItems(vector<int32_t> Items) {
	TXClear();

	for (int32_t &Item : Items)
		TXAddItem(Item);
}


/**
 * @brief Clear any previously written level/duration pairs that have not been sent.
 */
void RMT::TXClear() {
	OutputItems.clear();
}

/**
 * @brief Get items count ready for transmit.
 *
 * @param [out] Items in Queue
 *
 */
int16_t RMT::TXItemsCount() {
	int16_t ItemsCount = OutputItems.size() * 2;

	if (ItemsCount > 0)
		if (OutputItems[OutputItems.size()-1].duration1 == 0 && OutputItems[OutputItems.size()-1].level1)
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
 * @param [in] Pin 			Pin to send data
 * @param [in] Frequency 	Frequency of IR signal
 *
 */

void RMT::TXSend(gpio_num_t Pin, uint16_t Frequency) {
	if (Pin == GPIO_NUM_0) {
		ESP_LOGE("RMT", "GPIO for sending should be not null");
		return;
	}

	if (ChannelInfo.RxHandle != NULL)	RxStop(true);
	if (OutputItems.size() == 0) 		return;

	rmt_config_t Config;
	Config.rmt_mode                  = RMT_MODE_TX;
	Config.channel                   = RMT_CHANNEL_0;
	Config.gpio_num                  = Pin; //
	Config.mem_block_num             = 8;
	Config.clk_div                   = 80;
	Config.tx_config.loop_en         = 0;
	Config.tx_config.carrier_en      = 1; //?1
	Config.tx_config.idle_output_en  = 1;
	Config.tx_config.idle_level      = (rmt_idle_level_t)0;
	Config.tx_config.carrier_freq_hz = Frequency;
	Config.tx_config.carrier_level   = (rmt_carrier_level_t)1;
	Config.tx_config.carrier_duty_percent = 50;

	ESP_ERROR_CHECK(::rmt_config(&Config));
	ESP_ERROR_CHECK(::rmt_driver_install(RMT_CHANNEL_0, 0, 0));

	//ESP_ERROR_CHECK(rmt_set_source_clk(Channel, RMT_BASE));
	/*
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_36);
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_39);
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_34);
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_35);
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_9);
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_10);
	rmt_set_pin(Channel, RMT_MODE_TX, GPIO_NUM_1);
	*/

	PowerManagement::AddLock("RMTSend");

	if (OutputItems.back().duration0 > -30000 || OutputItems.back().duration1 > -30000)
		TXAddItem(-45000);

	::rmt_write_items(RMT_CHANNEL_0, &OutputItems[0] , OutputItems.size(), true);
	::rmt_wait_tx_done(RMT_CHANNEL_0, portMAX_DELAY);

	::rmt_driver_uninstall(RMT_CHANNEL_0);

	OutputItems.clear();

	PowerManagement::ReleaseLock("RMTSend");

	if (RMT::ChannelInfo.Pin != GPIO_NUM_0)
		RxStart(RMT::ChannelInfo.Pin, RMT::ChannelInfo.CallbackStart, RMT::ChannelInfo.CallbackBody, RMT::ChannelInfo.CallbackEnd);
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
int32_t RMT::PrepareBit(bool Bit, uint32_t Interval) {
	Interval = round(Interval / 10) * 10;
	return (!Bit) ? Interval : -Interval;
}

