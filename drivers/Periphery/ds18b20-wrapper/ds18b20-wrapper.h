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
#define MAX_DEVICES          (8)
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)

class DS18B20 {
	public:
		static vector<float> ReadData(gpio_num_t GPIO);
};

#endif
