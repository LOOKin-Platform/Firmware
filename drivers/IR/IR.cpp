/*
*    RMT.cpp
*    Class to using IR Remote Control module
*
*/

#include "IR.h"

static char tag[] = "IR";

map<rmt_channel_t,IR::IRChannelInfo> IR::ChannelsMap = {};

/**
 * @brief Set TX channel using for sending data.
 *
 * @param [in] Pin The GPIO pin on which the signal is sent/received.
 * @param [in] Channel The RMT channel to work with.
 */

void IR::SetTXChannel(gpio_num_t Pin, rmt_channel_t Channel) {

	rmt_config_t config;
	config.rmt_mode                  = RMT_MODE_TX;
	config.channel                   = Channel;
	config.gpio_num                  = Pin;
	config.mem_block_num             = 8-Channel;
	config.clk_div                   = 8;
	config.tx_config.loop_en         = 0;
	config.tx_config.carrier_en      = 0;
	config.tx_config.idle_output_en  = 1;
	config.tx_config.idle_level      = (rmt_idle_level_t)0;
	config.tx_config.carrier_freq_hz = 10000;
	config.tx_config.carrier_level   = (rmt_carrier_level_t)1;
	config.tx_config.carrier_duty_percent = 50;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(Channel, 0, 0));
}

/**
 * @brief Set RX channel using for receiving data.
 *
 * @param [in] Pin The GPIO pin on which the signal is sent/received.
 * @param [in] Channel The RMT channel to work with.
 * @param [in] Callback Function to handle received data.
 */

void IR::SetRXChannel(gpio_num_t Pin, rmt_channel_t Channel, IRChannelCallbackStart CallbackStart,
							IRChannelCallbackBody CallbackBody, IRChannelCallbackEnd CallbackEnd) {
	rmt_config_t config;
	config.rmt_mode                  = RMT_MODE_RX;
	config.channel                   = Channel;
	config.gpio_num                  = Pin;
	config.mem_block_num             = 8-Channel;
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
void IR::RXStart(rmt_channel_t Channel) {

	if (IR::ChannelsMap.count(Channel) > 0)
		if (IR::ChannelsMap[Channel].Handle > 0) {
			ESP_LOGE(tag,"Channel task already started");
			return;
		}

	IR::ChannelsMap[Channel].Handle = FreeRTOS::StartTask(RXTask, "RXReadTask" + static_cast<int>(Channel), (void *)(uint32_t)static_cast<int>(Channel), 2048);
}


/**
 * @brief Stop receiving for given channel.
 *
 * @param [in] Channel The RMT channel to work with.
 *
 */void IR::RXStop(rmt_channel_t Channel) {

	 ESP_ERROR_CHECK(::rmt_rx_stop(Channel));

	 if (IR::ChannelsMap.count(Channel) > 0)
		 if (IR::ChannelsMap[Channel].Handle == 0) {
			 ESP_LOGE(tag,"Channel task doesn't exist");
			 return;
		 }

	 FreeRTOS::DeleteTask(IR::ChannelsMap[Channel].Handle);
	 IR::ChannelsMap[Channel].Handle = 0;
}

void IR::RXTask(void *TaskData) {
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

		ESP_LOGE("REMOTE","Size: %d", rx_size);

		if(item)
		{
			int offset = 0;

			if (rx_size > 4) { // protect from sun ambient IR - light

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

				if (ChannelsMap[Channel].CallbackEnd != nullptr)
					ChannelsMap[Channel].CallbackEnd();
			}

			//after parsing the data, return spaces to ringbuffer.
			vRingbufferReturnItem(rb, (void*) item);
		}
	}
	vTaskDelete(NULL);
}

int16_t IR::PrepareBit(bool Bit, uint16_t Interval) {
	Interval = round(Interval / 10) * 10;
	//ESP_LOGI("OUTPUT", "%d", (!Bit) ? Interval : -Interval);
	return (!Bit) ? Interval : -Interval;
}

