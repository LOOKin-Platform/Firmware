/* Глобальные переменные */

#include <vector>

#include "WebServer.h"
#include "Device.h"
#include "Sensors.h"
#include "Commands.h"

extern WebServer_t        WebServer;
extern Device_t           Device;
extern vector<Sensor_t>   Sensors;
extern vector<Command_t>  Commands;

#define SWITCH_PIN_NUM    GPIO_NUM_17

#define OTA_SERVER_IP     "http://download.look-in.club/firmwares/"
#define OTA_SERVER_PORT   "80"
#define OTA_FILENAME      "0.2"
