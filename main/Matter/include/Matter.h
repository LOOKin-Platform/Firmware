#ifndef MATTER_MAIN
#define MATTER_MAIN

#include "FreeRTOSWrapper.h"

#include "Globals.h"

#include <stdio.h>
#include <string.h>
#include "Custom.h"

#include <common/CHIPDeviceManager.h>
#include <common/CommonDeviceCallbacks.h>

#include "NetworkCommissioningDriver.h"

#include "GenericDevice.hpp"

#define NVS_MATTER_AREA "matter"
class Matter {
	public:
		static inline FreeRTOS::Semaphore	
								m_scanFinished = FreeRTOS::Semaphore("MScanFinished");


		static void				WiFiSetMode(bool, string, string);

		static void				Init();
		static void				StartServer();
		
		static void 			WiFiScan();
		static void				WiFiConnectToAP(string SSID, string Password);

		static void				Start();
		static void				Stop();

		static void				AppServerRestart();

		static void				ResetPairs();
		static void				ResetData();

		static bool				IsEnabledForDevice();

		static void				Reboot();

        static MatterGenericDevice*
                                GetDeviceByDynamicIndex(uint16_t EndpointIndex);

	private:
		inline static 			vector<MatterGenericDevice *>	MatterDevices = vector<MatterGenericDevice *>();

		static int 				AddDeviceEndpoint(MatterGenericDevice *, EmberAfEndpointType *, const chip::Span<const EmberAfDeviceType> &, chip::EndpointId);
		static CHIP_ERROR 		RemoveDeviceEndpoint(MatterGenericDevice * dev);

		static int				BridgeIdentify		();
		static int				AccessoryIdentify	(uint16_t AID);

		static bool				On					(bool 		value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				Cursor				(uint8_t 	Key		, uint16_t AID);
		static bool				ActiveID			(uint8_t 	NewID	, uint16_t AID);
		static bool				Volume				(uint8_t 	Value	, uint16_t AID);
		static bool				HeaterCoolerState 	(uint8_t 	Value	, uint16_t AID);
		static bool				ThresholdTemperature(float 		Value	, uint16_t AID, bool IsCooling);
		static bool				RotationSpeed		(float		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				TargetFanState		(bool		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				SwingMode			(bool		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				TargetPosition		(uint8_t	Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);

		static bool				GetConfiguredName	(char*		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				SetConfiguredName	(char*		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);

		static void				StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value, bool Send = true);

//		static int 				WriteCallback	(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv);
		static void				StartServerInner(intptr_t context);

		static void				CreateAccessories(intptr_t context);
		static void				CreateRemoteBridge();
		static void				CreateWindowOpener();

		static void				Task(void *);

		static void				DeviceEventCallback(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

		static					TaskHandle_t TaskHandle;
		static bool				IsRunning;
		static bool				IsAP;

		static uint64_t			TVHIDLastUpdated;

		static vector<uint8_t*> BridgedAccessories;
};

#endif
