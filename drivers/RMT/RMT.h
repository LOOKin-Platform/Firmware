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
#include "Settings.h"

typedef void (*IRChannelCallbackStart)();
typedef bool (*IRChannelCallbackBody)(int16_t);
typedef void (*IRChannelCallbackEnd)();

#define RMT_CLK_DIV             80    								/*!< RMT counter clock divider */
#define RMT_TICK_10_US          (80000000/RMT_CLK_DIV/100000)		/*!< RMT counter value for 10 us.(Source clock is APB clock) */
#define rmt_item32_TIMEOUT_US   Settings.SensorsConfig.IR.Threshold /*!< RMT receiver timeout value(us) */

/**
 * @brief Drive the %RMT peripheral.
 */
class RMT {
	public:
		struct IRChannelInfo {
			TaskHandle_t			Handle 			= 0;
			gpio_num_t				Pin				= GPIO_NUM_0;
			uint16_t				Frequency		= 0;
			IRChannelCallbackStart 	CallbackStart 	= nullptr;
			IRChannelCallbackBody 	CallbackBody 	= nullptr;
			IRChannelCallbackEnd 	CallbackEnd 	= nullptr;
		};

		static void	Init();
		static void	Deinit();

		static void SetRXChannel(gpio_num_t Pin, rmt_channel_t Channel, IRChannelCallbackStart = nullptr, IRChannelCallbackBody = nullptr, IRChannelCallbackEnd = nullptr);
		static void ReceiveStart(rmt_channel_t Channel);
		static void ReceiveStop (rmt_channel_t Channel);

		static void SetTXChannel(gpio_num_t Pin, rmt_channel_t Channel, uint16_t Frequency);
		static void TXChangeFrequency(rmt_channel_t Channel, uint16_t Frequency);
		static void TXAddItem(int32_t);
		static void TXSetItems(vector<int32_t>);
		static void TXClear();
		static void IRAM_ATTR TXSend(rmt_channel_t Channel, uint16_t Frequency = 0);

		static int32_t PrepareBit(bool, uint32_t);

		static map<rmt_channel_t, IRChannelInfo> ChannelsMap;

		static vector<rmt_item32_t> OutputItems;
	private:
		static bool 				IsInited;
		static esp_pm_lock_handle_t APBLock, CPULock;

		static IRAM_ATTR void RXTask(void *);
};

#endif /* DRIVERS_IR_H_ */
