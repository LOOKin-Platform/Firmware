#ifndef GLOBALS_H
#define GLOBALS_H

/************************************/
/*         Global Variables         */
/************************************/

#include <vector>

#include "Automation.h"
#include "BluetoothServer.h"
#include "Commands.h"
#include "Device.h"
#include "Log.h"
#include "Network.h"
#include "Sensors.h"
#include "WebServer.h"
#include "WiFi.h"
#include "Storage.h"
#include "Settings.h"

extern Device_t					Device;
extern Network_t				Network;
extern Automation_t				Automation;
extern vector<Sensor_t*>		Sensors;
extern vector<Command_t*>		Commands;
extern Storage_t				Storage;

extern Settings_t				Settings;

extern WiFi_t					WiFi;
extern WebServer_t				WebServer;
extern BluetoothServer_t		BluetoothServer;

#endif
