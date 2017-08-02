#ifndef GLOBALS_H
#define GLOBALS_H

/************************************/
/*         Global Variables         */
/************************************/


#include "WebServer.h"

#include "Device.h"
#include "Network.h"
#include "Automation.h"
#include "Sensors.h"
#include "Commands.h"

#include <vector>

#include <WiFi/WiFi.h>

extern WiFi_t                     *WiFi;
extern WebServer_t                *WebServer;

extern Device_t                   *Device;
extern Network_t                  *Network;
extern Automation_t               *Automation;
extern vector<Sensor_t*>          Sensors;
extern vector<Command_t*>         Commands;

#endif
