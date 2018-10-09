#ifndef GLOBALS_H
#define GLOBALS_H

/************************************/
/*         Global Variables         */
/************************************/

#include <vector>

#include "Automation.h"
#include "Commands.h"
#include "Device.h"
#include "Log.h"
#include "Network.h"
#include "Sensors.h"
#include "Storage.h"

#include "WiFi.h"
#include "WebServer.h"
#include "BLEServer.h"
#include "BLEClient.h"
#include "Wireless.h"

#include "Settings.h"

extern Settings_t				Settings;

extern Device_t					Device;
extern Network_t				Network;
extern Automation_t				Automation;
extern vector<Sensor_t*>		Sensors;
extern vector<Command_t*>		Commands;
extern Storage_t				Storage;

extern WiFi_t					WiFi;
extern WebServer_t				WebServer;

#if defined(CONFIG_BT_ENABLED)
extern BLEServer_t				BLEServer;
extern BLEClient_t				BLEClient;
#endif /* Bluetooth enabled */

extern Wireless_t				Wireless;

#endif
