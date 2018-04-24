/*
*    RMT.h
*    Class to using IR Remote Control module
*
*/

#ifndef DRIVERS_IR_H_
#define DRIVERS_IR_H_

using namespace std;

#include <vector>
#include <map>
#include "math.h"

#include <driver/rmt.h>
#include <driver/rtc_io.h>
#include <esp_log.h>

#include "FreeRTOSWrapper.h"

typedef void (*IRChannelCallbackStart)();
typedef void (*IRChannelCallbackBody)(int16_t);
typedef void (*IRChannelCallbackEnd)();

#define RMT_CLK_DIV      		80    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    		(80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */
#define rmt_item32_TIMEOUT_US  	9500   /*!< RMT receiver timeout value(us) */

/**
 * @brief Drive the %RMT peripheral.
 */
class RMT {
	public:
		struct IRChannelInfo {
			TaskHandle_t				Handle 			= 0;
			IRChannelCallbackStart 	CallbackStart 	= nullptr;
			IRChannelCallbackBody 	CallbackBody 	= nullptr;
			IRChannelCallbackEnd 	CallbackEnd 		= nullptr;
		};

		static void SetRXChannel(gpio_num_t Pin, rmt_channel_t Channel, IRChannelCallbackStart = nullptr, IRChannelCallbackBody = nullptr, IRChannelCallbackEnd = nullptr);
		static void ReceiveStart(rmt_channel_t Channel);
		static void ReceiveStop (rmt_channel_t Channel);

		static void SetTXChannel(gpio_num_t Pin, rmt_channel_t Channel, uint16_t Frequent);
		static void AddItem(int32_t);
		static void SetItems(vector<int32_t>);
		static void Clear();
		static void Send(rmt_channel_t Channel);

		static int32_t PrepareBit(bool, uint32_t);

		static map<rmt_channel_t, IRChannelInfo> ChannelsMap;

		static vector<rmt_item32_t> OutputItems;
	private:
		static void RXTask(void *);
};

#endif /* DRIVERS_IR_H_ */
