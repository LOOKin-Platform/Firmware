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

		static bool		On					(bool 		value	, uint16_t AID, uint8_t Iterator = 0);
		static bool		Cursor				(uint8_t 	Key		, uint16_t AID);
		static bool		ActiveID			(uint8_t 	NewID	, uint16_t AID);
		static bool		Volume				(uint8_t 	Value	, uint16_t AID);
		static bool 	HeatingCoolingState (uint8_t 	Value	, uint16_t AID);
		static bool 	TargetTemperature 	(float 		Value	, uint16_t AID);
		static bool		RotationSpeed		(float		Value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);
		static bool		TargetFanState		(bool		Value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);
		static bool		SwingMode			(bool		Value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);

		static void		StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value);

		static int 		WriteCallback(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv);

		static void 	FillAccessories();
		static void 	Task(void *);

		static void		SetLastUpdatedForAID(uint16_t);
		static uint64_t	GetLastUpdatedForAID(uint16_t);
		static bool		IsRecentAction(uint16_t);

		static 			TaskHandle_t TaskHandle;

		static bool		IsRunning;

		static bool		IsAP;
		static string 	SSID;
		static string 	Password;

		static vector<hap_acc_t*> BridgedAccessories;
		static map<uint16_t, uint64_t> LastUpdated;
};

#endif
