/*
*    RMT.cpp
*    Class to using IR Remote Control module
*
*/

#include "RMT.h"

static char tag[] = "RMT";

map<rmt_channel_t,RMT::IRChannelInfo> RMT::ChannelsMap = {};
vector<rmt_item32_t> RMT::OutputItems = {};

/**
 * @brief Set RX channel using for receiving data.
 *
 * @param [in] Pin The GPIO pin on which the signal is sent/received.
 * @param [in] Channel The RMT channel to work with.
 * @param [in] Callback Function to handle received data.
 */

void RMT::SetRXChannel(gpio_num_t Pin, rmt_channel_t Channel, IRChannelCallbackStart CallbackStart,
							IRChannelCallbackBody CallbackBody, IRChannelCallbackEnd CallbackEnd) {
	rmt_config_t config;
	config.rmt_mode                  = RMT_MODE_RX;
	config.channel                   = Channel;
	config.gpio_num                  = Pin;
	config.mem_block_num             = 1;
	config.clk_div                   = RMT_CLK_DIV;
	config.rx_config.filter_en 		= true;
	config.rx_config.filter_ticks_thresh = 80;
	config.rx_config.idle_threshold 	= rmt_item32_TIMEOUT_US / 10 * (RMT_TICK_10_US);

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(Channel, 1000, 0));

	ChannelsMap[Channel].CallbackStart 	= CallbackStart;
	ChannelsMap[Channel].CallbackBody 	= CallbackBody;
	ChannelsMap[Channel].CallbackEnd 	= CallbackEnd;

	::rtc_gpio_isolate(Pin);
}

/**
 * @brief Start receiving for given channel.
 *
 * @param [in] Channel The RMT channel to work with.
 *
 */
void RMT::ReceiveStart(rmt_channel_t Channel) {
	if (RMT::ChannelsMap.count(Channel) > 0)
		if (RMT::ChannelsMap[Channel].Handle > 0) {
			ESP_LOGE(tag,"Channel task already started");
			return;
		}

	RMT::ChannelsMap[Channel].Handle = FreeRTOS::StartTask(RXTask, "RXReadTask" + static_cast<int>(Channel), (void *)(uint32_t)static_cast<int>(Channel), 4096);
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
		 if (RMT::ChannelsMap[Channel].Handle == 0) {
			 ESP_LOGE(tag,"Channel task doesn't exist");
			 return;
		 }

	 FreeRTOS::DeleteTask(RMT::ChannelsMap[Channel].Handle);
	 RMT::ChannelsMap[Channel].Handle = 0;
}

void RMT::RXTask(void *TaskData) {
	uint32_t ChannelNum = (uint32_t)TaskData;

    if ((ChannelNum < static_cast<int>(rmt_channel_t::RMT_CHANNEL_0)) ||
    		(ChannelNum > static_cast<int>(rmt_channel_t::RMT_CHANNEL_0))) {
    		ESP_LOGE(tag, "Channel number out of range");
    		return;
    }

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

			if (rx_size > 8) { // protect from sun ambient IR - light
				ESP_LOGE("RXTask","Size: %d", rx_size);

				if (ChannelsMap[Channel].CallbackStart != nullptr)
					ChannelsMap[Channel].CallbackStart();

				while (offset < rx_size)
				{
					if (ChannelsMap[Channel].CallbackBody != nullptr) {
						ChannelsMap[Channel].CallbackBody(PrepareBit((bool)(item+offset)->level0, (item+offset)->duration0));
						ChannelsMap[Channel].CallbackBody(PrepareBit((bool)(item+offset)->level1, (item+offset)->duration1));
					}
					offset++;
				}
			}

			if (ChannelsMap[Channel].CallbackEnd != nullptr)
				ChannelsMap[Channel].CallbackEnd();

			//after parsing the data, return spaces to ringbuffer.
			vRingbufferReturnItem(rb, (void*) item);
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

void RMT::SetTXChannel(gpio_num_t Pin, rmt_channel_t Channel, uint16_t Frequent) {
	rmt_config_t config;
	config.rmt_mode                  = RMT_MODE_TX;
	config.channel                   = Channel;
	config.gpio_num                  = Pin;
	config.mem_block_num             = 1;
	config.clk_div                   = RMT_CLK_DIV;
	config.tx_config.loop_en         = 0;
	config.tx_config.carrier_en      = 1; //?1
	config.tx_config.idle_output_en  = 1;
	config.tx_config.idle_level      = (rmt_idle_level_t)0;
	config.tx_config.carrier_freq_hz = Frequent;
	config.tx_config.carrier_level   = (rmt_carrier_level_t)1;
	config.tx_config.carrier_duty_percent = 50;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(Channel, 0, 0));

	//::rtc_gpio_isolate(Pin);
}

/**
 * @brief Add a level/duration to the transaction to be written.
 *
 * @param [in] Bit Bit to add to the queue. Negative means 0, positive - 1. Modul means duration of the bit
 */
void RMT::AddItem(int32_t Bit) {
	if (OutputItems.size() == 0 || (OutputItems.back()).duration1 != 0) {
		rmt_item32_t Item;
		Item.level0 		= (Bit > 0 ) ? 1 : 0 ;
		Item.duration0 	= abs(Bit);
		Item.level1 		= 0;
		Item.duration1 	= 0;

		OutputItems.push_back(Item);
	}
	else {
		OutputItems.back().level1	= (Bit > 0 ) ? 1 : 0 ;
		OutputItems.back().duration1	= abs(Bit);
	}
}

void RMT::SetItems(vector<int32_t> Items) {
	Clear();

	for (int32_t &Item : Items)
		AddItem(Item);
}


/**
 * @brief Clear any previously written level/duration pairs that have not been sent.
 */
void RMT::Clear() {
	OutputItems.clear();
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

void RMT::Send(rmt_channel_t Channel) {
	if (OutputItems.size() > 0) {
		AddItem(-1000);

		::rmt_write_items(Channel, &OutputItems[0] , OutputItems.size(), true);
	    ::rmt_wait_tx_done(Channel, portMAX_DELAY);

	    Clear();
	}
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

