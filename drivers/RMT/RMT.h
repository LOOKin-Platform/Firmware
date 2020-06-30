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
		class IRChannelInfo_t {
			public:
				TaskHandle_t			RxHandle 		= NULL;
				gpio_num_t				Pin				= GPIO_NUM_0;
				uint16_t				Frequency		= 0;
				IRChannelCallbackStart 	CallbackStart 	= nullptr;
				IRChannelCallbackBody 	CallbackBody 	= nullptr;
				IRChannelCallbackEnd 	CallbackEnd 	= nullptr;
		};

		static void 	RxStart(gpio_num_t Pin, IRChannelCallbackStart = nullptr, IRChannelCallbackBody = nullptr, IRChannelCallbackEnd = nullptr);
		static void 	RxStop (bool SaveData = false);

		static void 	TXAddItem(int32_t);
		static void 	TXAddItemExact(int32_t);
		static void 	TXSetItems(vector<int32_t>);
		static void 	TXClear();
		static int16_t 	TXItemsCount();
		static void IRAM_ATTR TXSend(gpio_num_t Pin, uint16_t Frequency);

		static int32_t 	PrepareBit(bool, uint32_t);

		static IRChannelInfo_t ChannelInfo;

		static vector<rmt_item32_t> OutputItems;
	private:
		static IRAM_ATTR void RXTask(void *);
};

#endif /* DRIVERS_IR_H_ */
