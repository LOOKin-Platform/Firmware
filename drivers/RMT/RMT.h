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
#include <esp_pm.h>

#include "FreeRTOSWrapper.h"
#include "PowerManagement.h"

#include "Settings.h"

typedef void (*IRChannelCallbackStart)();
typedef bool (*IRChannelCallbackBody)(int16_t);
typedef void (*IRChannelCallbackEnd)();

#define RMT_CLK_DIV             1    								/*!< RMT counter clock divider */
#define RMT_TICK_10_US          (1000000/RMT_CLK_DIV/100000)		/*!< RMT counter value for 10 us.(Source clock is APB clock) */
#define rmt_item32_TIMEOUT_US   Settings.SensorsConfig.IR.Threshold /*!< RMT receiver timeout value(us) */

/**
 * @brief Drive the %RMT peripheral.
 */
class RMT {
	public:
		struct IRChannelInfo {
			TaskHandle_t			Handle 			= NULL;
			gpio_num_t				Pin				= GPIO_NUM_0;
			uint16_t				Frequency		= 0;
			IRChannelCallbackStart 	CallbackStart 	= nullptr;
			IRChannelCallbackBody 	CallbackBody 	= nullptr;
			IRChannelCallbackEnd 	CallbackEnd 	= nullptr;
		};

		static void		Init();
		static void		Deinit();

		static void		SetRXChannel(gpio_num_t Pin, rmt_channel_t Channel, IRChannelCallbackStart = nullptr, IRChannelCallbackBody = nullptr, IRChannelCallbackEnd = nullptr);
		static void		ReceiveStart(rmt_channel_t Channel);
		static void		ReceiveStop (rmt_channel_t Channel);
		static void		UnsetRXChannel(rmt_channel_t Channel);

		static void		SetTXChannel(vector<gpio_num_t> GPIO, rmt_channel_t Channel, uint16_t Frequency);
		static void		UnsetTXChannel(rmt_channel_t Channel);

		static void		TXAddItem(int32_t);
		static void		TXAddItemExact(int32_t);
		static void		TXSetItems(vector<int32_t>);
		static void		TXClear();

		static int16_t	TXItemsCount();

		static void		TXSend(rmt_channel_t Channel);

		static int32_t	PrepareBit(bool, uint32_t);

		static inline map<rmt_channel_t, IRChannelInfo> ChannelsMap = {};

		static inline rmt_item32_t 		OutputItems[400] 	= { 0 };
		static inline uint16_t			OutputItemsSize 	= 0;
	private:
		static inline bool 				IsInited 			= false;

		static IRAM_ATTR void RXTask(void *);
};

#endif /* DRIVERS_IR_H_ */
