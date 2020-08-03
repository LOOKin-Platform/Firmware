/*
*    HomeKit.cpp
*    Class to handle HomeKit data
*
*/

#include "HomeKit.h"

const char Tag[] 	= "HomeKit";
// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.
// This example code is in the Public Domain (or CC0 licensed, at your option.)
//
// Unless required by applicable law or agreed to in writing, this
// software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied.

#include <signal.h>
static bool requestedFactoryReset = false;
static bool clearPairings = false;

bool HomeKit::ShouldServerRestartFlag = false;

TaskHandle_t 	HomeKit::TaskHandle 	= NULL;
char 			HomeKit::MDNSName[9] 	= "lookin\0";


PlatfromStruct HomeKit::Platform = {0};

/**
 * HomeKit accessory server that hosts the accessory.
 */
static HAPAccessoryServerRef accessoryServer;


bool IRAM_ATTR HomeKit::IsSupported() {
	if (Settings.eFuse.Type == 0x81 && Settings.eFuse.Model > 1)
		return true;

	if (Settings.eFuse.DeviceID == 0x00000002)
		return true;

	return false;
}


/**
 * Initialize global platform objects.
 */
void IRAM_ATTR HomeKit::InitializePlatform() {
    // Key-value store.
	const HAPPlatformKeyValueStoreOptions KeyValueStoreOptions = {
		.part_name 			= "nvs",
		.namespace_prefix 	= "hap",
		.read_only 			= false
	};

    HAPPlatformKeyValueStoreCreate(&Platform.keyValueStore, &KeyValueStoreOptions);
    Platform.hapPlatform.keyValueStore = &Platform.keyValueStore;

	const HAPPlatformKeyValueStoreOptions FactoryKeyValueStoreOptions = {
		.part_name 			= "fctry",
		.namespace_prefix 	= "hap",
		.read_only 			= true
	};

    HAPPlatformKeyValueStoreCreate(&Platform.factoryKeyValueStore, &FactoryKeyValueStoreOptions);

    // Accessory setup manager. Depends on key-value store.
    static HAPPlatformAccessorySetup accessorySetup;

    const HAPPlatformAccessorySetupOptions HAPSetupOptions = {
    	.keyValueStore = &Platform.factoryKeyValueStore
    };

    HAPPlatformAccessorySetupCreate(&accessorySetup, &HAPSetupOptions);
    Platform.hapPlatform.accessorySetup = &accessorySetup;

    // Initialise Wi-Fi
    //app_wifi_init();

    const HAPPlatformTCPStreamManagerOptions PlatformTCPStreamManagerOptions = {
    	/* Listen on all available network interfaces. */
    	.port = 0 /* Listen on unused port number from the ephemeral port range. */,
		.maxConcurrentTCPStreams = 15,
    };

    // TCP stream manager.
    HAPPlatformTCPStreamManagerCreate(&Platform.tcpStreamManager, &PlatformTCPStreamManagerOptions);

    // Service discovery.
	::memcpy(MDNSName, Device.IDToString().data(), Device.IDToString().size());
    const HAPPlatformServiceDiscoveryOptions PlatformServiceDiscoveryOptions = {
    	.hostName = MDNSName
    };

    static HAPPlatformServiceDiscovery serviceDiscovery;
    HAPPlatformServiceDiscoveryCreate(&serviceDiscovery, &PlatformServiceDiscoveryOptions);
    Platform.hapPlatform.ip.serviceDiscovery = &serviceDiscovery;

#if HAVE_MFI_HW_AUTH
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthCreate(&Platform.mfiHWAuth);
#endif

#if HAVE_MFI_HW_AUTH
    platform.hapPlatform.authentication.mfiHWAuth = &Platform.mfiHWAuth;
#endif

    // Software Token provider. Depends on key-value store.
    const HAPPlatformMFiTokenAuthOptions PlatformMFiTokenAuthOptions = {
    	.keyValueStore = &Platform.keyValueStore
    };

    HAPPlatformMFiTokenAuthCreate(&Platform.mfiTokenAuth, &PlatformMFiTokenAuthOptions);

    // Run loop.
    const HAPPlatformRunLoopOptions PlatformRunLoopOptions = { .keyValueStore = &Platform.keyValueStore};
    HAPPlatformRunLoopCreate(&PlatformRunLoopOptions);

    Platform.hapAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;

    Platform.hapPlatform.authentication.mfiTokenAuth =
            HAPPlatformMFiTokenAuthIsProvisioned(&Platform.mfiTokenAuth) ? &Platform.mfiTokenAuth : NULL;

    Platform.hapAccessoryServerCallbacks.handleUpdatedState = HomeKit::HandleUpdatedState;
}

/**
 * Deinitialize global platform objects.
 */
void IRAM_ATTR HomeKit::DeinitializePlatform() {
#if HAVE_MFI_HW_AUTH
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthRelease(&Platform.mfiHWAuth);
#endif

    // TCP stream manager.
    HAPPlatformTCPStreamManagerRelease(&Platform.tcpStreamManager);

    HomeKitApp::Deinitialize();

    // Run loop.
    HAPPlatformRunLoopRelease();
}

/**
 * Restore platform specific factory settings.
 */
void RestorePlatformFactorySettings(void) {
}

/**
 * Either simply passes State handling to app, or processes Factory Reset
 */
void IRAM_ATTR HomeKit::HandleUpdatedState(HAPAccessoryServerRef* _Nonnull server, void* _Nullable context) {
    if (HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle && requestedFactoryReset) {
        HAPPrecondition(server);

        HAPError err;

        HAPLogInfo(&kHAPLog_Default, "A factory reset has been requested.");

        // Purge app state.
        err = HAPPlatformKeyValueStorePurgeDomain(&Platform.keyValueStore, ((HAPPlatformKeyValueStoreDomain) 0x00));
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Reset HomeKit state.
        err = HAPRestoreFactorySettings(&Platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Restore platform specific factory settings.
        RestorePlatformFactorySettings();

        // De-initialize App.
        HomeKitApp::Release();

        requestedFactoryReset = false;

        // Re-initialize App.
        HomeKitApp::Create(server, &Platform.keyValueStore);

        // Restart accessory server.
        HomeKitApp::AppAccessoryServerStart();
        return;
    }
    else if (HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle && clearPairings)
    {
        HAPError err;
        err = HAPRemoveAllPairings(&Platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        HomeKitApp::AppAccessoryServerStart();
    }
    else
    {
        HomeKitApp::AccessoryServerHandleUpdatedState(server, context);
    }
}

void IRAM_ATTR HomeKit::InitializeIP() {
    // Prepare accessory server storage.
    static HAPIPSession ipSessions[kHAPIPSessionStorage_MinimumNumElements];
    static uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_MinimumInboundBufferSize];
    static uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_MinimumOutboundBufferSize];
    static HAPIPEventNotificationRef ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
    }
    static HAPIPReadContextRef ipReadContexts[kAttributeCount];
    static HAPIPWriteContextRef ipWriteContexts[kAttributeCount];
    static uint8_t ipScratchBuffer[kHAPIPSession_MinimumScratchBufferSize];

    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .readContexts = ipReadContexts,
        .numReadContexts = HAPArrayCount(ipReadContexts),
        .writeContexts = ipWriteContexts,
        .numWriteContexts = HAPArrayCount(ipWriteContexts),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer }
    };

    Platform.hapAccessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    Platform.hapAccessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;

    Platform.hapPlatform.ip.tcpStreamManager = &Platform.tcpStreamManager;

    // Connect to Wi-Fi
    //app_wifi_connect();
}

void IRAM_ATTR HomeKit::Task(void *)
{
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Initialize global platform objects.
    InitializePlatform();
    InitializeIP();

    // Perform Application-specific initalizations such as setting up callbacks
    // and configure any additional unique platform dependencies
    HomeKitApp::Initialize(&Platform.hapAccessoryServerOptions, &Platform.hapPlatform, &Platform.hapAccessoryServerCallbacks);

    // Initialize accessory server.
    HAPAccessoryServerCreate(
            &accessoryServer,
            &Platform.hapAccessoryServerOptions,
            &Platform.hapPlatform,
            &Platform.hapAccessoryServerCallbacks,
            /* context: */ NULL);

    // Create app object.
    HomeKitApp::Create(&accessoryServer, &Platform.keyValueStore);

    // Start accessory server for App.
    HomeKitApp::AppAccessoryServerStart();

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.

    // Cleanup.
    HomeKitApp::Release();
    HAPAccessoryServerRelease(&accessoryServer);
    DeinitializePlatform();
}

void IRAM_ATTR HomeKit::Start()
{
	TaskHandle = FreeRTOS::StartTask(HomeKit::Task, "HomeKit task", nullptr, 6 * 1024, 7);
}

void IRAM_ATTR HomeKit::Stop() {
	/* no-op */
}

void IRAM_ATTR HomeKit::AppServerRestart() {
	if (ShouldServerRestartFlag) return;

	ShouldServerRestartFlag = true;

	int AppServerRebootSignal = APP_SERVER_REBOOT_SIGNAL;
	HAPPlatformRunLoopScheduleCallback(HomeKit::StopSheduleCallback, &AppServerRebootSignal, sizeof(AppServerRebootSignal));
}

void IRAM_ATTR HomeKit::StopSheduleCallback(void* _Nullable context, size_t contextSize) {
    int Signal = *((int*) context);

    if (Signal == APP_SERVER_REBOOT_SIGNAL)
    	HomeKitApp::AppAccessoryServerStop();
}
