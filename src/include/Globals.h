/* Глобальные переменные */

#include <vector>

#include "WebServer.h"
#include "Device.h"
#include "Sensors.h"
#include "Commands.h"

#include <drivers/WiFi/WiFi.h>

extern WiFi_t               *WiFi;
extern WebServer_t          *WebServer;

extern Device_t             *Device;
extern vector<Sensor_t*>    Sensors;
extern vector<Command_t*>   Commands;

#define OTA_SERVER_IP       "http://download.look-in.club/firmwares/"
#define OTA_SERVER_PORT     "80"
#define OTA_FILENAME        "0.2"

// Имена устройств по-умолчанию
#define DEFAULTNAME_PLUG    "Plug"

// Commands and Sensors Pin Map
#define SWITCH_PLUG_PIN_NUM GPIO_NUM_2
