#ifndef HOMEKIT_SDK
#define HOMEKIT_SDK

#include "FreeRTOSWrapper.h"

#include "Globals.h"

#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include <hap_platform_httpd.h>

#include <stdio.h>
#include <string.h>

class HomeKit {
	public:
		class AccessoryData_t {
			public:
				char Name	[32];
				char Model	[2];
				char ID		[8];
				hap_acc_t 	*Handle = NULL;

				AccessoryData_t(string Name, string Model, string ID);
		};

		static void 	WiFiSetMode(bool, string, string);

		static void 	Start();
		static void 	Stop();

		static void 	AppServerRestart();

		static void		ResetData();

	private:
		static int 		BridgeIdentify		(hap_acc_t *ha);
		static int 		AccessoryIdentify	(hap_acc_t *ha);

		static bool		On				(bool value, uint16_t aid);
		static bool		Cursor			(uint8_t Key, uint16_t aid);
		static bool		ActiveID		(uint8_t NewID, uint16_t aid);
		static bool		Volume			(uint8_t Value, uint16_t aid);

		static int 		WriteCallback(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv);

		static void 	FillAccessories();
		static void 	Task(void *);

		static 			TaskHandle_t TaskHandle;

		static bool		IsRunning;

		static bool		IsAP;
		static string 	SSID;
		static string 	Password;

		static vector<hap_acc_t*> BridgedAccessories;
};

#endif
