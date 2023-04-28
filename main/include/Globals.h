#ifndef GLOBALS_H
#define GLOBALS_H

/************************************/
/*         Global Variables         */
/************************************/

#include <vector>

#include "Automation.h"
#include "Commands.h"
#include "Device.h"
#include "Network.h"
#include "Sensors.h"
#include "Storage.h"
#include "Data.h"

#include "WiFi.h"
#include "Webserver.h"
#include "BLEServerGeneric.h"
#include "BLEClient.h"
#include "LocalMQTT.h"
#include "RemoteControl.h"
#include "Wireless.h"
#include "Settings.h"

extern Settings_t				Settings;

extern Device_t					Device;
extern Network_t				Network;
extern Automation_t				Automation;
extern vector<Sensor_t*>		Sensors;
extern vector<Command_t*>		Commands;
extern Storage_t				Storage;
extern DataEndpoint_t			*Data;

extern WiFi_t					WiFi;
extern WebServer_t				WebServer;

#if defined(CONFIG_BT_ENABLED)
extern BLEServerGeneric_t		MyBLEServer;
extern BLEClient_t				MyBLEClient;
#endif /* Bluetooth enabled */

extern Wireless_t				Wireless;
extern RemoteControl_t			RemoteControl;
extern LocalMQTT_t				LocalMQTT;

#endif
