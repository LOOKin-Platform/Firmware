#ifndef DS18B20_DEVICE
#define DS18B20_DEVICE

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#include <vector>

using namespace std;

#define GPIO_DS18B20_0       (GPIO_NUM_15)
#define MAX_DEVICES          (1) // 8
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_9_BIT) // DS18B20_RESOLUTION_12_BIT

class DS18B20 {
	private:
		static bool IsInited;

	public:
		static void Init(gpio_num_t GPIO);
		static void Deinit();

		static vector<float> ReadData();
};

#endif
