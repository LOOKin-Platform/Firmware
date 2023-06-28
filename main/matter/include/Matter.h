#ifndef MATTER_MAIN
#define MATTER_MAIN

#include "FreeRTOSWrapper.h"

#include <cstdio>
#include <cstring>
#include "Custom.h"

#include "NetworkCommissioningDriver.h"

#include "GenericDevice.h"
#include "Thermostat.h"

#define NVS_MATTER_AREA "matter"

class Matter {
	public:
		static inline FreeRTOS::Semaphore	
								m_scanFinished = FreeRTOS::Semaphore("MScanFinished");

		static void				Init();
		static void				StartServer();

		static void				Start();
		static void				Stop();

		static void				AppServerRestart();

		static void				ResetPairs();
		static void				ResetData();

		static bool				IsEnabledForDevice();

		static void				Reboot();

        static MatterGenericDevice*
                                GetDeviceByDynamicIndex(uint16_t EndpointIndex);

		static MatterGenericDevice*
								GetBridgedAccessoryByType(MatterGenericDevice::DeviceTypeEnum, string UUID = "");

		static void				StatusACUpdateIRSend(string UUID, uint16_t Codeset, uint8_t FunctionID, uint8_t Value, bool Send = true);

        /// <summary>
        /// Send IR Command to device
        /// </summary>
        static void             SendIRWrapper(string UUID, string params);

	private:
		inline static 			vector<MatterGenericDevice *>	MatterDevices = vector<MatterGenericDevice *>();

		static int 				AddDeviceEndpoint(MatterGenericDevice *, EmberAfEndpointType *, const chip::Span<const EmberAfDeviceType> &, chip::EndpointId);
		static CHIP_ERROR 		RemoveDeviceEndpoint(MatterGenericDevice * dev);

		static int				BridgeIdentify		();
		static int				AccessoryIdentify	(uint16_t AID);

		static bool				On					(bool 		value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				HeaterCoolerState 	(uint8_t 	Value	, uint16_t AID);
		static bool				ThresholdTemperature(float 		Value	, uint16_t AID, bool IsCooling);
		static bool				RotationSpeed		(float		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				TargetFanState		(bool		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				SwingMode			(bool		Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);
		static bool				TargetPosition		(uint8_t	Value	, uint16_t AID, uint8_t *Char, uint8_t Iterator = 0);

		static void				StartServerInner(intptr_t context);

		static void				CreateAccessories(intptr_t context);
		static void				CreateRemoteBridge();
		static void				CreateWindowOpener();

		static void				Task(void *);

		static void				DeviceEventCallback(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

		static bool				IsRunning;

		static vector<uint8_t*> BridgedAccessories;
};

#endif
