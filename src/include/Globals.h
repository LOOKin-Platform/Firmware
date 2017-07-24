/* Глобальные переменные */

#include <vector>

#include "WebServer.h"

#include "Device.h"
#include "Sensors.h"
#include "Commands.h"
#include "Network.h"

#include <WiFi/WiFi.h>

extern WiFi_t               *WiFi;
extern WebServer_t          *WebServer;

extern Device_t             *Device;
extern Network_t            *Network;
extern vector<Sensor_t*>    Sensors;
extern vector<Command_t*>   Commands;

#define FIRMWARE_VERSION    "0.61"

#define OTA_SERVER_IP       "92.63.203.74"
#define OTA_SERVER_HOST     "download.look-in.club"
#define OTA_SERVER_PORT     "80"
#define OTA_URL_PREFIX      "/firmwares/"

#define WIFI_AP_NAME        "Beeline_" + Device->TypeToString() + "_" + Device->ID
#define WIFI_AP_PASSWORD    Device->ID

#define UDP_SERVER_PORT     61201
#define UDP_PACKET_PREFIX   "BEELINE:"

// Имена устройств по-умолчанию
#define DEFAULT_NAME_PLUG   "Plug"

// Commands and Sensors Pin Map
#define SWITCH_PLUG_PIN_NUM GPIO_NUM_2
