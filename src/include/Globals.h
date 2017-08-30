#ifndef GLOBALS_H
#define GLOBALS_H

/************************************/
/*         Global Variables         */
/************************************/

#include <vector>

#include "Device.h"
#include "Network.h"
#include "Automation.h"
#include "Sensors.h"
#include "Commands.h"

#include "WebServer.h"
#include "WiFi.h"

extern Device_t                   *Device;
extern Network_t                  *Network;
extern Automation_t               *Automation;
extern vector<Sensor_t*>          Sensors;
extern vector<Command_t*>         Commands;

extern WiFi_t                     *WiFi;
extern WebServer_t                *WebServer;

#endif
