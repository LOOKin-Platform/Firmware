/*
*    Sleep.h
*    Class describes SoC sleep modes
*
*/

#ifndef DRIVERS_SLEEP_H
#define DRIVERS_SLEEP_H

#include <esp_log.h>

#if defined(CONFIG_BT_ENABLED)
#include <esp_bt.h>
#include <esp_bt_main.h>
#endif /* Bluetooth enabled */

#include <driver/rtc_io.h>
#include <esp_wifi.h>
#include <esp_sleep.h>

using namespace std;

class Sleep {
  public:
    Sleep();
    ~Sleep();

    static esp_sleep_wakeup_cause_t GetWakeUpReason();

    static void SetTimerInterval(uint16_t Interval);
    static void SetGPIOWakeupSource(gpio_num_t gpio_num, int level);

    static void LightSleep(uint16_t Interval = 0);
};

#endif //DRIVERS_SLEEP_H
