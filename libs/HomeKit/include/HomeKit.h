/*
*    HomeKit.h
*    Class to handle HomeKit data
*
*/

#ifndef LIBSHOMEKIT_H
#define LIBSHOMEKIT_H

#define HOMEKIT_HAVE_NFC false

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <stdio.h>
#include <string.h>

#include "App.h"
#include "DB.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "HAP.h"
#include "HAPPlatform+Init.h"
#include "HAPPlatformAccessorySetup+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"
#include "HAPPlatformKeyValueStore+Init.h"
#include "HAPPlatformMFiHWAuth+Init.h"
#include "HAPPlatformMFiTokenAuth+Init.h"
#include "HAPPlatformRunLoop+Init.h"

#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"

typedef struct {
	HAPPlatformKeyValueStore 	keyValueStore;
	HAPPlatformKeyValueStore 	factoryKeyValueStore;
	HAPAccessoryServerOptions 	hapAccessoryServerOptions;
	HAPPlatform 				hapPlatform;
	HAPAccessoryServerCallbacks hapAccessoryServerCallbacks;

	#if HOMEKIT_HAVE_NFC
	HAPPlatformAccessorySetupNFC setupNFC;
	#endif

	HAPPlatformTCPStreamManager tcpStreamManager;

	HAPPlatformMFiHWAuth 		mfiHWAuth;
	HAPPlatformMFiTokenAuth 	mfiTokenAuth;
} PlatfromStruct;

class HomeKit {
	public:
		static void InitializePlatform();
		static void DeinitializePlatform();

		static void InitializeIP();

		static void HandleUpdatedState(HAPAccessoryServerRef* _Nonnull server, void* _Nullable context);

		static void Start();

	private:
		static void Task(void *);
		static PlatfromStruct Platform;
};

#endif //LIBSHOMEKIT_H
