#ifndef DRIVERS_OTA_H_
#define DRIVERS_OTA_H_

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "nvs_flash.h"

#include "include/Globals.h"

#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

class OTA {
  public:
	   OTA();
	   ~OTA();
	   void Update();

  private:
    bool HttpConnect();
    static int ReadUntil(char *, char, int);
    static bool ReadPastHttpHeader(char*, int, esp_ota_handle_t);

    void __attribute__((noreturn)) task_fatal_error();
};

#endif
