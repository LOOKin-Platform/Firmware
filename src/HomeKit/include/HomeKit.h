#ifndef HOMEKIT_SDK
#define HOMEKIT_SDK

#include "FreeRTOSWrapper.h"

#include "Globals.h"

#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include <hap_platform_httpd.h>

#include <stdio.h>
#include <string.h>
#include "Custom.h"

#define	NVS_HOMEKIT_AREA 		"homekit"
#define NVS_HOMEKIT_AREA_MODE	"mode"

class HomeKit {
	public:
		enum ModeEnum { NONE = 0, BASIC, EXPERIMENTAL };

		class AccessoryData_t {
			public:
				char Name	[32];
				char Model	[2];
				char ID		[8];
				hap_acc_t 	*Handle = NULL;

				AccessoryData_t(string Name, string Model, string ID);
		};

		static void 			WiFiSetMode(bool, string, string);

		static void				Init();

		static void 			Start();
		static void 			Stop();

		static void 			AppServerRestart();

		static void				ResetPairs();
		static void				ResetData();

		static bool				IsEnabledForDevice();
		static bool				IsExperimentalMode();
	private:
		static int 				BridgeIdentify		(hap_acc_t *ha);
		static int 				AccessoryIdentify	(hap_acc_t *ha);

		static hap_status_t		On					(bool 		value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);
		static bool				Cursor				(uint8_t 	Key		, uint16_t AID);
		static bool				ActiveID			(uint8_t 	NewID	, uint16_t AID);
		static bool				Volume				(uint8_t 	Value	, uint16_t AID);
		static bool 			HeaterCoolerState 	(uint8_t 	Value	, uint16_t AID);
		static bool 			ThresholdTemperature(float 		Value	, uint16_t AID, bool IsCooling);
		static bool				RotationSpeed		(float		Value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);
		static bool				TargetFanState		(bool		Value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);
		static bool				SwingMode			(bool		Value	, uint16_t AID, hap_char_t *Char, uint8_t Iterator = 0);

		static void				StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value, bool Send = true);

		static int 				WriteCallback(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv);

		static hap_cid_t		FillAccessories();

		static hap_cid_t 		FillRemoteACOnly(hap_acc_t *Accessory);
		static hap_cid_t 		FillRemoteBridge(hap_acc_t *Accessory);


		static void 			Task(void *);

		static 					TaskHandle_t TaskHandle;
		static bool				IsRunning;
		static bool				IsAP;
		static ModeEnum			Mode;

		static uint64_t			VolumeLastUpdated;

		static vector<hap_acc_t*> BridgedAccessories;
};

#endif
